#include <algorithm>
#include <cmath>
#include <physics/system.h>

namespace eXngine::Physics
{
    namespace
    {
        using EXMATH::vec3;

        // Part of the overlap pushed out per step. Higher values separate faster but make
        // resting bodies jitter.
        constexpr EXFLOAT kPositionCorrection = 0.2f;
        // Overlap that is left alone, so resting contacts do not flicker on and off.
        constexpr EXFLOAT kPenetrationSlop = 0.005f;
        // Slower impacts do not bounce: otherwise a resting body would never come to rest.
        constexpr EXFLOAT kRestitutionThreshold = 0.5f;
        // Shapes this close count as touching (speculative contacts): the solver lets them
        // close the gap but not pass it, so resting contacts do not switch on and off.
        constexpr EXFLOAT kContactMargin = 0.02f;
        // Contact points closer than this to one of the last step are the same point.
        constexpr EXFLOAT kWarmStartDistance = 0.025f;

        EXUINT64 PairKey(EXUINT a, EXUINT b)
        {
            return (static_cast<EXUINT64>(a) << 32) | static_cast<EXUINT64>(b);
        }

        // Inertia of a solid shape around its local axes (the diagonal of the inertia tensor).
        vec3 ComputeInertia(const eXshape &shape, EXFLOAT mass)
        {
            if (shape.Type == eXcolliderShape::Sphere)
                return vec3(0.4f * mass * shape.Radius * shape.Radius);

            const vec3 squared = shape.HalfExtents * shape.HalfExtents;
            return (mass / 3.0f) * vec3(squared.y + squared.z, squared.x + squared.z, squared.x + squared.y);
        }

        // Unit vectors at right angles to normal and to each other.
        void ComputeTangents(const vec3 &normal, vec3 tangents[2])
        {
            if (std::abs(normal.x) >= 0.57735f)
                tangents[0] = EXMATH::normalize(vec3(normal.y, -normal.x, 0.0f));
            else
                tangents[0] = EXMATH::normalize(vec3(0.0f, normal.z, -normal.y));

            tangents[1] = EXMATH::cross(normal, tangents[0]);
        }
    }

    eXphysicsSystem::eXphysicsSystem(const Types::eXvec3 &gravity) : m_Gravity(gravity)
    {
    }

    void eXphysicsSystem::OnUpdate(World::eXworld &world, EXFLOAT deltaTime)
    {
        if (m_bPaused)
        {
            m_fAccumulator = 0.0f;
            return;
        }

        m_fAccumulator += deltaTime;

        for (EXUINT32 steps = 0; m_fAccumulator >= m_fFixedTimeStep && steps < m_nMaxSubSteps; ++steps)
        {
            Step(world, m_fFixedTimeStep);
            m_fAccumulator -= m_fFixedTimeStep;
        }

        // Too slow to keep up: drop the time that is left rather than owing it.
        if (m_fAccumulator >= m_fFixedTimeStep)
            m_fAccumulator = 0.0f;
    }

    void eXphysicsSystem::OnDetach(World::eXworld &)
    {
        m_Bodies.clear();
        m_Constraints.clear();
        m_Contacts.clear();
        m_ImpulseCache.clear();
        m_fAccumulator = 0.0f;
    }

    void eXphysicsSystem::Step(World::eXworld &world, EXFLOAT timeStep)
    {
        GatherBodies(world);

        // 1. Gravity and forces change the velocities.
        const vec3 gravity(m_Gravity);

        for (Body &body : m_Bodies)
        {
            if (body.RigidBody == EXN_NULL_HANDLE)
                continue;

            if (body.InverseMass > 0.0f)
            {
                const eXrigidBodyComponent &rigidBody = *body.RigidBody;

                body.LinearVelocity += (gravity * rigidBody.GravityScale + vec3(rigidBody.Force) * body.InverseMass) * timeStep;
                body.AngularVelocity += body.InverseInertia * vec3(rigidBody.Torque) * timeStep;

                body.LinearVelocity *= 1.0f / (1.0f + timeStep * std::max(rigidBody.LinearDamping, 0.0f));
                body.AngularVelocity *= 1.0f / (1.0f + timeStep * std::max(rigidBody.AngularDamping, 0.0f));
            }

            body.RigidBody->Force = Types::eXvec3(0.0f, 0.0f, 0.0f);
            body.RigidBody->Torque = Types::eXvec3(0.0f, 0.0f, 0.0f);
        }

        // 2. Contacts, 3. impulses, 4. overlaps, 5. positions.
        FindContacts(timeStep);
        SolveVelocities();
        SolvePushes();
        IntegratePositions(timeStep);
        StoreResults();
    }

    void eXphysicsSystem::GatherBodies(World::eXworld &world)
    {
        m_Bodies.clear();

        const auto add = [&](EXUINT id, const eXcolliderComponent *collider)
        {
            auto *entity = world.GetEntity(id);
            auto *transform = entity != EXN_NULL_HANDLE ? entity->GetComponent<Component::eXtransformComponent>() : EXN_NULL_HANDLE;

            if (transform == EXN_NULL_HANDLE)
                return;

            Body body;
            body.Entity = id;
            body.Transform = transform;
            body.RigidBody = entity->GetComponent<eXrigidBodyComponent>();
            body.Collider = collider;

            if (collider != EXN_NULL_HANDLE)
            {
                body.Shape = MakeShape(*collider, *transform);
            }
            else
            {
                // A body without a collider still needs some inertia to rotate: a box of its scale.
                body.Shape = MakeShape(eXcolliderComponent::Box(), *transform);
            }

            body.Position = body.Shape.Center;
            body.Orientation = body.Shape.Orientation;
            body.PivotOffset = EXMATH::conjugate(body.Orientation) * (vec3(transform->Position) - body.Position);
            body.Bounded = collider != EXN_NULL_HANDLE && ComputeBounds(body.Shape, body.MinBounds, body.MaxBounds);

            if (const eXrigidBodyComponent *rigidBody = body.RigidBody; rigidBody != EXN_NULL_HANDLE && rigidBody->Type != eXbodyType::Static)
            {
                body.LinearVelocity = vec3(rigidBody->LinearVelocity);
                body.AngularVelocity = vec3(rigidBody->AngularVelocity);

                // Planes are infinite: they cannot fall.
                if (rigidBody->IsDynamic() && body.Shape.Type != eXcolliderShape::Plane)
                {
                    const vec3 inertia = ComputeInertia(body.Shape, rigidBody->Mass);

                    body.InverseMass = 1.0f / rigidBody->Mass;
                    body.InverseInertiaLocal = vec3(inertia.x > 0.0f ? 1.0f / inertia.x : 0.0f,
                                                    inertia.y > 0.0f ? 1.0f / inertia.y : 0.0f,
                                                    inertia.z > 0.0f ? 1.0f / inertia.z : 0.0f);
                    // The inertia rotates with the body: R * I^-1 * R^T.
                    body.InverseInertia = body.Shape.Axes * EXMATH::mat3(
                                                                EXMATH::vec3(body.InverseInertiaLocal.x, 0.0f, 0.0f),
                                                                EXMATH::vec3(0.0f, body.InverseInertiaLocal.y, 0.0f),
                                                                EXMATH::vec3(0.0f, 0.0f, body.InverseInertiaLocal.z)) *
                                          EXMATH::transpose(body.Shape.Axes);
                }
            }

            m_Bodies.push_back(body);
        };

        for (auto &[id, collider] : world.GetComponents<eXcolliderComponent>())
            add(id, &collider);

        for (auto &[id, rigidBody] : world.GetComponents<eXrigidBodyComponent>())
        {
            auto *entity = world.GetEntity(id);

            if (entity != EXN_NULL_HANDLE && !entity->HasComponent<eXcolliderComponent>())
                add(id, EXN_NULL_HANDLE);
        }
    }

    static void ApplyImpulse(auto &a, auto &b, const vec3 &armA, const vec3 &armB, const vec3 &impulse)
    {
        a.LinearVelocity -= impulse * a.InverseMass;
        a.AngularVelocity -= a.InverseInertia * EXMATH::cross(armA, impulse);
        b.LinearVelocity += impulse * b.InverseMass;
        b.AngularVelocity += b.InverseInertia * EXMATH::cross(armB, impulse);
    }

    static vec3 RelativeVelocity(const auto &a, const auto &b, const vec3 &armA, const vec3 &armB)
    {
        return (b.LinearVelocity + EXMATH::cross(b.AngularVelocity, armB)) - (a.LinearVelocity + EXMATH::cross(a.AngularVelocity, armA));
    }

    static void ApplyPush(auto &a, auto &b, const vec3 &armA, const vec3 &armB, const vec3 &impulse)
    {
        a.PushVelocity -= impulse * a.InverseMass;
        a.PushAngularVelocity -= a.InverseInertia * EXMATH::cross(armA, impulse);
        b.PushVelocity += impulse * b.InverseMass;
        b.PushAngularVelocity += b.InverseInertia * EXMATH::cross(armB, impulse);
    }

    static vec3 RelativePushVelocity(const auto &a, const auto &b, const vec3 &armA, const vec3 &armB)
    {
        return (b.PushVelocity + EXMATH::cross(b.PushAngularVelocity, armB)) - (a.PushVelocity + EXMATH::cross(a.PushAngularVelocity, armA));
    }

    void eXphysicsSystem::FindContacts(EXFLOAT timeStep)
    {
        m_Constraints.clear();
        m_Contacts.clear();

        // Every pair is tested: simple, and fast enough for a few hundred bodies. A larger
        // world would first sort the bounds (sweep and prune) or use a grid / tree.
        for (EXUINT32 i = 0; i < m_Bodies.size(); ++i)
        {
            for (EXUINT32 j = i + 1; j < m_Bodies.size(); ++j)
            {
                // Always the same order for the same pair, so the impulse cache finds it again.
                const EXUINT32 indexA = m_Bodies[i].Entity < m_Bodies[j].Entity ? i : j;
                const EXUINT32 indexB = indexA == i ? j : i;
                Body &a = m_Bodies[indexA];
                Body &b = m_Bodies[indexB];

                if (a.Collider == EXN_NULL_HANDLE || b.Collider == EXN_NULL_HANDLE)
                    continue;

                // Two bodies that cannot be pushed have nothing to solve.
                if (a.InverseMass == 0.0f && b.InverseMass == 0.0f)
                    continue;

                // Bounds further apart than the contact margin cannot touch.
                if (a.Bounded && b.Bounded &&
                    (EXMATH::any(EXMATH::greaterThan(a.MinBounds, b.MaxBounds + kContactMargin)) ||
                     EXMATH::any(EXMATH::greaterThan(b.MinBounds, a.MaxBounds + kContactMargin))))
                    continue;

                eXmanifold manifold;

                if (!Collide(a.Shape, b.Shape, manifold, kContactMargin))
                    continue;

                Constraint constraint;
                constraint.A = indexA;
                constraint.B = indexB;
                constraint.Normal = manifold.Normal;
                constraint.Friction = std::sqrt(std::max(a.Collider->Friction, 0.0f) * std::max(b.Collider->Friction, 0.0f));
                constraint.Restitution = std::max(a.Collider->Restitution, b.Collider->Restitution);
                constraint.PointCount = manifold.PointCount;
                ComputeTangents(constraint.Normal, constraint.Tangents);

                const auto cached = m_ImpulseCache.find(PairKey(a.Entity, b.Entity));
                // Each impulse of last step goes to one point at most, or two new points close
                // to the same old one would both start with all of its impulse.
                EXBOOL used[EXN_MAX_MANIFOLD_POINTS] = {};

                for (EXUINT32 k = 0; k < manifold.PointCount; ++k)
                {
                    ContactPoint &point = constraint.Points[k];
                    const vec3 &position = manifold.Points[k].Position;

                    point.ArmA = position - a.Position;
                    point.ArmB = position - b.Position;
                    point.LocalA = EXMATH::conjugate(a.Orientation) * point.ArmA;
                    point.Penetration = manifold.Points[k].Penetration;

                    // The impulse that stops the bodies along a direction is 1 / (their
                    // inverse masses + how much the impulse also spins them).
                    const auto effectiveMass = [&](const vec3 &direction)
                    {
                        const vec3 spinA = EXMATH::cross(a.InverseInertia * EXMATH::cross(point.ArmA, direction), point.ArmA);
                        const vec3 spinB = EXMATH::cross(b.InverseInertia * EXMATH::cross(point.ArmB, direction), point.ArmB);
                        const EXFLOAT sum = a.InverseMass + b.InverseMass + EXMATH::dot(direction, spinA + spinB);

                        return sum > 0.0f ? 1.0f / sum : 0.0f;
                    };

                    point.NormalMass = effectiveMass(constraint.Normal);
                    point.TangentMass[0] = effectiveMass(constraint.Tangents[0]);
                    point.TangentMass[1] = effectiveMass(constraint.Tangents[1]);

                    // Not touching yet: approach at most as fast as closes the gap in this
                    // step. Touching: bounce back when hit hard enough.
                    const EXFLOAT approachSpeed = EXMATH::dot(RelativeVelocity(a, b, point.ArmA, point.ArmB), constraint.Normal);

                    if (point.Penetration < 0.0f)
                        point.Bias = point.Penetration / timeStep;
                    else if (approachSpeed < -kRestitutionThreshold)
                        point.Bias = -constraint.Restitution * approachSpeed;

                    // Overlaps are fixed by SolvePushes, not by the velocities.
                    point.PushBias = kPositionCorrection / timeStep * std::max(point.Penetration - kPenetrationSlop, 0.0f);

                    // Warm starting: begin with the impulse this point needed last step.
                    if (cached != m_ImpulseCache.end())
                    {
                        for (EXUINT32 c = 0; c < cached->second.size() && c < EXN_MAX_MANIFOLD_POINTS; ++c)
                        {
                            const CachedImpulse &impulse = cached->second[c];
                            const vec3 offset = impulse.LocalA - point.LocalA;

                            if (!used[c] && EXMATH::dot(offset, offset) < kWarmStartDistance * kWarmStartDistance)
                            {
                                used[c] = true;
                                point.NormalImpulse = impulse.NormalImpulse;
                                point.TangentImpulse[0] = impulse.TangentImpulse[0];
                                point.TangentImpulse[1] = impulse.TangentImpulse[1];
                                break;
                            }
                        }
                    }

                    m_Contacts.push_back({a.Entity, b.Entity, position, constraint.Normal, point.Penetration});
                }

                m_Constraints.push_back(constraint);
            }
        }

        // Warm starting goes last: applied while preparing, the impulses of one contact
        // would change the approach speeds of the next and look like impacts to bounce from.
        for (const Constraint &constraint : m_Constraints)
        {
            for (EXUINT32 k = 0; k < constraint.PointCount; ++k)
            {
                const ContactPoint &point = constraint.Points[k];

                ApplyImpulse(m_Bodies[constraint.A], m_Bodies[constraint.B], point.ArmA, point.ArmB,
                             constraint.Normal * point.NormalImpulse +
                                 constraint.Tangents[0] * point.TangentImpulse[0] +
                                 constraint.Tangents[1] * point.TangentImpulse[1]);
            }
        }
    }

    void eXphysicsSystem::SolveVelocities()
    {
        // Each contact is solved on its own, which disturbs the ones solved before it:
        // repeating the loop converges toward impulses that satisfy all of them.
        for (EXUINT32 iteration = 0; iteration < m_nIterations; ++iteration)
        {
            for (Constraint &constraint : m_Constraints)
            {
                Body &a = m_Bodies[constraint.A];
                Body &b = m_Bodies[constraint.B];

                for (EXUINT32 k = 0; k < constraint.PointCount; ++k)
                {
                    ContactPoint &point = constraint.Points[k];

                    // Friction stops sliding, up to Friction times the normal impulse (Coulomb).
                    const EXFLOAT maxFriction = constraint.Friction * point.NormalImpulse;

                    for (EXINT t = 0; t < 2; ++t)
                    {
                        const EXFLOAT slideSpeed = EXMATH::dot(RelativeVelocity(a, b, point.ArmA, point.ArmB), constraint.Tangents[t]);
                        const EXFLOAT previous = point.TangentImpulse[t];

                        point.TangentImpulse[t] = EXMATH::clamp(previous - slideSpeed * point.TangentMass[t], -maxFriction, maxFriction);
                        ApplyImpulse(a, b, point.ArmA, point.ArmB, constraint.Tangents[t] * (point.TangentImpulse[t] - previous));
                    }

                    // The normal impulse may only push: the total is clamped, not each change,
                    // so later iterations can take back what earlier ones overdid.
                    const EXFLOAT normalSpeed = EXMATH::dot(RelativeVelocity(a, b, point.ArmA, point.ArmB), constraint.Normal);
                    const EXFLOAT previous = point.NormalImpulse;

                    point.NormalImpulse = std::max(previous + (point.Bias - normalSpeed) * point.NormalMass, 0.0f);
                    ApplyImpulse(a, b, point.ArmA, point.ArmB, constraint.Normal * (point.NormalImpulse - previous));
                }
            }
        }
    }

    void eXphysicsSystem::SolvePushes()
    {
        // Like the normal impulses, but on the push velocities: pushing an overlap apart
        // through the real velocities would leave the bodies flying apart afterwards.
        for (EXUINT32 iteration = 0; iteration < m_nIterations; ++iteration)
        {
            for (Constraint &constraint : m_Constraints)
            {
                Body &a = m_Bodies[constraint.A];
                Body &b = m_Bodies[constraint.B];

                for (EXUINT32 k = 0; k < constraint.PointCount; ++k)
                {
                    ContactPoint &point = constraint.Points[k];

                    if (point.PushBias <= 0.0f && point.PushImpulse <= 0.0f)
                        continue;

                    const EXFLOAT pushSpeed = EXMATH::dot(RelativePushVelocity(a, b, point.ArmA, point.ArmB), constraint.Normal);
                    const EXFLOAT previous = point.PushImpulse;

                    point.PushImpulse = std::max(previous + (point.PushBias - pushSpeed) * point.NormalMass, 0.0f);
                    ApplyPush(a, b, point.ArmA, point.ArmB, constraint.Normal * (point.PushImpulse - previous));
                }
            }
        }
    }

    void eXphysicsSystem::IntegratePositions(EXFLOAT timeStep)
    {
        for (Body &body : m_Bodies)
        {
            if (body.RigidBody == EXN_NULL_HANDLE || body.RigidBody->Type == eXbodyType::Static)
                continue;

            body.Position += (body.LinearVelocity + body.PushVelocity) * timeStep;

            // dq/dt = 0.5 * (0, w) * q
            const vec3 angularVelocity = body.AngularVelocity + body.PushAngularVelocity;
            const EXMATH::quat spin(0.0f, angularVelocity.x, angularVelocity.y, angularVelocity.z);
            body.Orientation = EXMATH::normalize(body.Orientation + (spin * body.Orientation) * (0.5f * timeStep));
        }
    }

    void eXphysicsSystem::StoreResults()
    {
        for (Body &body : m_Bodies)
        {
            if (body.RigidBody == EXN_NULL_HANDLE || body.RigidBody->Type == eXbodyType::Static)
                continue;

            body.Transform->Position = body.Position + body.Orientation * body.PivotOffset;
            body.Transform->Rotation = body.Orientation;
            body.RigidBody->LinearVelocity = body.LinearVelocity;
            body.RigidBody->AngularVelocity = body.AngularVelocity;
        }

        // Pairs that no longer touch drop out of the cache.
        m_ImpulseCache.clear();

        for (const Constraint &constraint : m_Constraints)
        {
            auto &impulses = m_ImpulseCache[PairKey(m_Bodies[constraint.A].Entity, m_Bodies[constraint.B].Entity)];

            for (EXUINT32 k = 0; k < constraint.PointCount; ++k)
            {
                const ContactPoint &point = constraint.Points[k];
                impulses.push_back({point.LocalA, point.NormalImpulse, {point.TangentImpulse[0], point.TangentImpulse[1]}});
            }
        }
    }

    EXBOOL eXphysicsSystem::Raycast(World::eXworld &world, const Types::eXvec3 &origin, const Types::eXvec3 &direction,
                                    EXFLOAT maxDistance, eXraycastHit &hit)
    {
        const EXFLOAT length = EXMATH::length(vec3(direction));

        if (length < 1e-8f)
            return false;

        const vec3 rayOrigin(origin);
        const vec3 rayDirection = vec3(direction) / length;
        EXBOOL found = false;

        for (auto &[id, collider] : world.GetComponents<eXcolliderComponent>())
        {
            auto *entity = world.GetEntity(id);
            const auto *transform = entity != EXN_NULL_HANDLE ? entity->GetComponent<Component::eXtransformComponent>() : EXN_NULL_HANDLE;

            if (transform == EXN_NULL_HANDLE)
                continue;

            EXFLOAT distance = 0.0f;
            vec3 normal(0.0f);

            // Each hit shortens the ray, so only closer colliders can hit next.
            if (Physics::Raycast(MakeShape(collider, *transform), rayOrigin, rayDirection, found ? hit.Distance : maxDistance, distance, normal))
            {
                hit.Entity = id;
                hit.Distance = distance;
                hit.Point = rayOrigin + rayDirection * distance;
                hit.Normal = normal;
                found = true;
            }
        }

        return found;
    }
}
