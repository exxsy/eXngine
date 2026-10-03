#include <algorithm>
#include <cfloat>
#include <cmath>
#include <physics/collision.h>

namespace eXngine::Physics
{
    namespace
    {
        using EXMATH::vec3;

        void AddPoint(eXmanifold &manifold, const vec3 &position, EXFLOAT penetration)
        {
            if (manifold.PointCount < EXN_MAX_MANIFOLD_POINTS)
                manifold.Points[manifold.PointCount++] = {position, penetration};
        }

        EXFLOAT SignOf(EXFLOAT value) { return value < 0.0f ? -1.0f : 1.0f; }

        EXBOOL SphereSphere(const eXshape &a, const eXshape &b, EXFLOAT margin, eXmanifold &manifold)
        {
            const vec3 d = b.Center - a.Center;
            const EXFLOAT radii = a.Radius + b.Radius;
            const EXFLOAT distanceSquared = EXMATH::dot(d, d);

            if (distanceSquared >= (radii + margin) * (radii + margin))
                return false;

            const EXFLOAT distance = std::sqrt(distanceSquared);
            // Same center: any direction separates them.
            const vec3 normal = distance > 1e-6f ? d / distance : vec3(0.0f, 1.0f, 0.0f);
            const EXFLOAT penetration = radii - distance;

            manifold.Normal = normal;
            AddPoint(manifold, a.Center + normal * (a.Radius - penetration * 0.5f), penetration);
            return true;
        }

        EXBOOL SpherePlane(const eXshape &sphere, const eXshape &plane, EXFLOAT margin, eXmanifold &manifold)
        {
            // Signed distance of the center in front of the plane.
            const EXFLOAT distance = EXMATH::dot(plane.Normal, sphere.Center - plane.Center);

            if (distance >= sphere.Radius + margin)
                return false;

            manifold.Normal = -plane.Normal;
            // Halfway between the deepest point of the sphere and the plane.
            AddPoint(manifold, sphere.Center - plane.Normal * ((sphere.Radius + distance) * 0.5f), sphere.Radius - distance);
            return true;
        }

        EXBOOL SphereBox(const eXshape &sphere, const eXshape &box, EXFLOAT margin, eXmanifold &manifold)
        {
            // In the box's space the box is axis aligned: the closest point is a clamp.
            const vec3 local = EXMATH::transpose(box.Axes) * (sphere.Center - box.Center);
            const vec3 closest = EXMATH::clamp(local, -box.HalfExtents, box.HalfExtents);
            const vec3 outside = local - closest;
            const EXFLOAT distanceSquared = EXMATH::dot(outside, outside);

            if (distanceSquared > (sphere.Radius + margin) * (sphere.Radius + margin))
                return false;

            if (distanceSquared > 1e-12f)
            {
                const EXFLOAT distance = std::sqrt(distanceSquared);
                const vec3 normal = -(box.Axes * (outside / distance));
                const vec3 surface = box.Center + box.Axes * closest;
                const vec3 deepest = sphere.Center + normal * sphere.Radius;

                manifold.Normal = normal;
                AddPoint(manifold, (surface + deepest) * 0.5f, sphere.Radius - distance);
                return true;
            }

            // The center is inside the box: leave through the nearest face.
            EXINT axis = 0;
            EXFLOAT depth = box.HalfExtents[0] - std::abs(local[0]);

            for (EXINT k = 1; k < 3; ++k)
            {
                const EXFLOAT faceDepth = box.HalfExtents[k] - std::abs(local[k]);

                if (faceDepth < depth)
                {
                    depth = faceDepth;
                    axis = k;
                }
            }

            const vec3 faceNormal = box.Axes[axis] * SignOf(local[axis]);

            manifold.Normal = -faceNormal;
            AddPoint(manifold, sphere.Center + faceNormal * ((depth - sphere.Radius) * 0.5f), sphere.Radius + depth);
            return true;
        }

        vec3 BoxCorner(const eXshape &box, EXINT index)
        {
            const vec3 signs((index & 1) ? 1.0f : -1.0f, (index & 2) ? 1.0f : -1.0f, (index & 4) ? 1.0f : -1.0f);
            return box.Center + box.Axes * (signs * box.HalfExtents);
        }

        EXBOOL BoxPlane(const eXshape &box, const eXshape &plane, EXFLOAT margin, eXmanifold &manifold)
        {
            // Every corner behind the plane (or within margin of it) touches it.
            for (EXINT i = 0; i < 8; ++i)
            {
                const vec3 corner = BoxCorner(box, i);
                const EXFLOAT distance = EXMATH::dot(plane.Normal, corner - plane.Center);

                if (distance < margin)
                    AddPoint(manifold, corner - plane.Normal * (distance * 0.5f), -distance);
            }

            manifold.Normal = -plane.Normal;
            return manifold.PointCount > 0;
        }

        // How far the shadows of the boxes on axis overlap; < 0 means axis separates them.
        EXFLOAT BoxOverlap(const eXshape &a, const eXshape &b, const vec3 &axis, const vec3 &centers)
        {
            EXFLOAT projected = -std::abs(EXMATH::dot(centers, axis));

            for (EXINT i = 0; i < 3; ++i)
            {
                projected += a.HalfExtents[i] * std::abs(EXMATH::dot(a.Axes[i], axis));
                projected += b.HalfExtents[i] * std::abs(EXMATH::dot(b.Axes[i], axis));
            }

            return projected;
        }

        // Keeps the part of polygon in front of (dot(normal, x) <= offset). Sutherland-Hodgman.
        EXUINT32 ClipPolygon(const vec3 *input, EXUINT32 count, const vec3 &normal, EXFLOAT offset, vec3 *output)
        {
            EXUINT32 outputCount = 0;

            for (EXUINT32 i = 0; i < count; ++i)
            {
                const vec3 &from = input[i];
                const vec3 &to = input[(i + 1) % count];
                const EXFLOAT fromDistance = EXMATH::dot(normal, from) - offset;
                const EXFLOAT toDistance = EXMATH::dot(normal, to) - offset;

                if (fromDistance <= 0.0f)
                    output[outputCount++] = from;

                // The edge crosses the plane.
                if ((fromDistance <= 0.0f) != (toDistance <= 0.0f))
                    output[outputCount++] = from + (to - from) * (fromDistance / (fromDistance - toDistance));
            }

            return outputCount;
        }

        // Face contact: clips the face of incident most opposed to normal against the side
        // planes of the face of reference along normal (normal = +-reference axis).
        void ClipFaces(const eXshape &reference, EXINT referenceAxis, const vec3 &normal, const eXshape &incident, EXFLOAT margin, eXmanifold &manifold)
        {
            const vec3 referenceFace = reference.Center + normal * reference.HalfExtents[referenceAxis];

            EXINT incidentAxis = 0;
            EXFLOAT best = -1.0f;

            for (EXINT i = 0; i < 3; ++i)
            {
                const EXFLOAT alignment = std::abs(EXMATH::dot(incident.Axes[i], normal));

                if (alignment > best)
                {
                    best = alignment;
                    incidentAxis = i;
                }
            }

            // The incident face looks back at the reference face.
            const EXFLOAT side = EXMATH::dot(incident.Axes[incidentAxis], normal) > 0.0f ? -1.0f : 1.0f;
            const vec3 center = incident.Center + incident.Axes[incidentAxis] * (side * incident.HalfExtents[incidentAxis]);
            const vec3 p = incident.Axes[(incidentAxis + 1) % 3] * incident.HalfExtents[(incidentAxis + 1) % 3];
            const vec3 q = incident.Axes[(incidentAxis + 2) % 3] * incident.HalfExtents[(incidentAxis + 2) % 3];

            // Each clip adds at most one corner: 4 + 4 clips = 8.
            vec3 polygon[EXN_MAX_MANIFOLD_POINTS] = {center + p + q, center - p + q, center - p - q, center + p - q};
            vec3 clipped[EXN_MAX_MANIFOLD_POINTS];
            EXUINT32 count = 4;

            for (const EXINT axis : {(referenceAxis + 1) % 3, (referenceAxis + 2) % 3})
            {
                for (const EXFLOAT sign : {1.0f, -1.0f})
                {
                    const vec3 sideNormal = reference.Axes[axis] * sign;
                    const EXFLOAT offset = EXMATH::dot(sideNormal, reference.Center) + reference.HalfExtents[axis];

                    count = ClipPolygon(polygon, count, sideNormal, offset, clipped);
                    std::copy(clipped, clipped + count, polygon);
                }
            }

            // The points of the clipped face that are below the reference face (or within margin).
            eXcontactPoint candidates[EXN_MAX_MANIFOLD_POINTS];
            EXUINT32 candidateCount = 0;

            for (EXUINT32 i = 0; i < count; ++i)
            {
                const EXFLOAT depth = EXMATH::dot(normal, referenceFace - polygon[i]);

                if (depth > -margin)
                    candidates[candidateCount++] = {polygon[i] + normal * (depth * 0.5f), depth};
            }

            if (candidateCount <= 4)
            {
                for (EXUINT32 i = 0; i < candidateCount; ++i)
                    AddPoint(manifold, candidates[i].Position, candidates[i].Penetration);

                return;
            }

            // Four points hold a face as well as eight, and clipping nearly parallel edges
            // leaves points almost on top of each other. Keep the deepest point, the one
            // furthest from it, and the two that span the largest area on either side of them.
            EXUINT32 chosen[4] = {0, 0, 0, 0};

            for (EXUINT32 i = 1; i < candidateCount; ++i)
            {
                if (candidates[i].Penetration > candidates[chosen[0]].Penetration)
                    chosen[0] = i;
            }

            const vec3 first = candidates[chosen[0]].Position;
            EXFLOAT furthest = -1.0f;

            for (EXUINT32 i = 0; i < candidateCount; ++i)
            {
                const vec3 offset = candidates[i].Position - first;

                if (EXMATH::dot(offset, offset) > furthest)
                {
                    furthest = EXMATH::dot(offset, offset);
                    chosen[1] = i;
                }
            }

            const vec3 line = candidates[chosen[1]].Position - first;
            EXFLOAT mostPositive = 0.0f, mostNegative = 0.0f;
            EXUINT32 pointCount = 2;

            chosen[2] = chosen[3] = chosen[0];

            for (EXUINT32 i = 0; i < candidateCount; ++i)
            {
                // Twice the signed area of the triangle (first, second, i), seen along the normal.
                const EXFLOAT area = EXMATH::dot(EXMATH::cross(line, candidates[i].Position - first), normal);

                if (area > mostPositive)
                {
                    mostPositive = area;
                    chosen[2] = i;
                }
                else if (area < mostNegative)
                {
                    mostNegative = area;
                    chosen[3] = i;
                }
            }

            if (mostPositive > 0.0f)
                chosen[pointCount++] = chosen[2];

            if (mostNegative < 0.0f)
                chosen[pointCount++] = chosen[3];

            for (EXUINT32 i = 0; i < pointCount; ++i)
                AddPoint(manifold, candidates[chosen[i]].Position, candidates[chosen[i]].Penetration);
        }

        // Separating axis test over the 3 + 3 face axes and the 9 edge / edge axes.
        EXBOOL BoxBox(const eXshape &a, const eXshape &b, EXFLOAT margin, eXmanifold &manifold)
        {
            const vec3 centers = b.Center - a.Center;

            EXFLOAT faceA = FLT_MAX, faceB = FLT_MAX, edge = FLT_MAX;
            EXINT axisA = 0, axisB = 0;
            vec3 edgeAxis(0.0f);
            EXINT edgeA = 0, edgeB = 0;

            for (EXINT i = 0; i < 3; ++i)
            {
                const EXFLOAT overlapA = BoxOverlap(a, b, a.Axes[i], centers);
                const EXFLOAT overlapB = BoxOverlap(a, b, b.Axes[i], centers);

                if (overlapA < -margin || overlapB < -margin)
                    return false;

                if (overlapA < faceA)
                {
                    faceA = overlapA;
                    axisA = i;
                }

                if (overlapB < faceB)
                {
                    faceB = overlapB;
                    axisB = i;
                }
            }

            for (EXINT i = 0; i < 3; ++i)
            {
                for (EXINT j = 0; j < 3; ++j)
                {
                    vec3 axis = EXMATH::cross(a.Axes[i], b.Axes[j]);
                    const EXFLOAT length = EXMATH::length(axis);

                    // Parallel edges: the face axes already cover this direction.
                    if (length < 1e-4f)
                        continue;

                    axis /= length;
                    const EXFLOAT overlap = BoxOverlap(a, b, axis, centers);

                    if (overlap < -margin)
                        return false;

                    if (overlap < edge)
                    {
                        edge = overlap;
                        edgeAxis = axis;
                        edgeA = i;
                        edgeB = j;
                    }
                }
            }

            // Prefer faces (stable resting contacts) unless an edge is clearly better, and
            // A over B, so the choice does not flicker between nearly equal axes. The overlaps
            // can be negative (within margin), so the tolerance is a distance: a small part
            // of the boxes' size.
            const EXFLOAT smallest = std::min(EXMATH::min(a.HalfExtents.x, EXMATH::min(a.HalfExtents.y, a.HalfExtents.z)),
                                              EXMATH::min(b.HalfExtents.x, EXMATH::min(b.HalfExtents.y, b.HalfExtents.z)));
            const EXFLOAT tolerance = 0.05f * smallest;
            const EXBOOL useFaceB = faceB < faceA - 0.5f * tolerance;
            const EXFLOAT face = useFaceB ? faceB : faceA;

            if (edge < face - tolerance)
            {
                const vec3 normal = edgeAxis * SignOf(EXMATH::dot(edgeAxis, centers));

                // The edges of A and B that stick out furthest toward each other.
                vec3 pointA = a.Center, pointB = b.Center;

                for (EXINT k = 0; k < 3; ++k)
                {
                    if (k != edgeA)
                        pointA += a.Axes[k] * (a.HalfExtents[k] * SignOf(EXMATH::dot(a.Axes[k], normal)));

                    if (k != edgeB)
                        pointB -= b.Axes[k] * (b.HalfExtents[k] * SignOf(EXMATH::dot(b.Axes[k], normal)));
                }

                // Closest points of the two edge lines.
                const vec3 &directionA = a.Axes[edgeA];
                const vec3 &directionB = b.Axes[edgeB];
                const vec3 r = pointA - pointB;
                const EXFLOAT cosine = EXMATH::dot(directionA, directionB);
                const EXFLOAT denominator = 1.0f - cosine * cosine;
                const EXFLOAT f = EXMATH::dot(directionB, r);
                EXFLOAT s = denominator > 1e-6f ? (cosine * f - EXMATH::dot(directionA, r)) / denominator : 0.0f;
                EXFLOAT t = cosine * s + f;

                s = EXMATH::clamp(s, -a.HalfExtents[edgeA], a.HalfExtents[edgeA]);
                t = EXMATH::clamp(t, -b.HalfExtents[edgeB], b.HalfExtents[edgeB]);

                manifold.Normal = normal;
                AddPoint(manifold, ((pointA + directionA * s) + (pointB + directionB * t)) * 0.5f, edge);
                return true;
            }

            if (useFaceB)
            {
                // Reference face on B, facing A.
                const vec3 normal = b.Axes[axisB] * -SignOf(EXMATH::dot(b.Axes[axisB], centers));
                manifold.Normal = -normal;
                ClipFaces(b, axisB, normal, a, margin, manifold);
            }
            else
            {
                const vec3 normal = a.Axes[axisA] * SignOf(EXMATH::dot(a.Axes[axisA], centers));
                manifold.Normal = normal;
                ClipFaces(a, axisA, normal, b, margin, manifold);
            }

            return manifold.PointCount > 0;
        }
    }

    eXshape MakeShape(const eXcolliderComponent &collider, const Types::eXtransform &transform)
    {
        const vec3 scale = EXMATH::vec3(transform.Scale);
        const vec3 absoluteScale = EXMATH::abs(scale);

        eXshape shape;
        shape.Type = collider.Shape;
        shape.Orientation = EXMATH::normalize(EXMATH::quat(transform.Rotation));
        shape.Axes = EXMATH::mat3_cast(shape.Orientation);
        shape.Center = vec3(transform.Position) + shape.Axes * (vec3(collider.Offset) * scale);
        shape.HalfExtents = EXMATH::abs(vec3(collider.HalfExtents)) * absoluteScale;
        shape.Radius = std::abs(collider.Radius) * std::max(absoluteScale.x, std::max(absoluteScale.y, absoluteScale.z));

        // Normals scale by the inverse of the scale (the inverse transpose of the matrix).
        const vec3 normal = shape.Axes * (vec3(collider.Normal) / EXMATH::max(scale * scale, vec3(1e-12f)) * scale);
        const EXFLOAT length = EXMATH::length(normal);
        shape.Normal = length > 1e-6f ? normal / length : shape.Axes[1];

        return shape;
    }

    EXBOOL ComputeBounds(const eXshape &shape, EXMATH::vec3 &min, EXMATH::vec3 &max)
    {
        vec3 extents(0.0f);

        switch (shape.Type)
        {
        case eXcolliderShape::Sphere:
            extents = vec3(shape.Radius);
            break;
        case eXcolliderShape::Box:
            // The rotated box fits in sum(|axis| * half extent).
            for (EXINT i = 0; i < 3; ++i)
                extents += EXMATH::abs(shape.Axes[i]) * shape.HalfExtents[i];
            break;
        case eXcolliderShape::Plane:
            return false;
        }

        min = shape.Center - extents;
        max = shape.Center + extents;
        return true;
    }

    EXBOOL Collide(const eXshape &a, const eXshape &b, eXmanifold &manifold, EXFLOAT margin)
    {
        // The tests below expect the shapes in enum order: swap, then flip the normal.
        if (static_cast<EXUINT32>(a.Type) > static_cast<EXUINT32>(b.Type))
        {
            if (!Collide(b, a, manifold, margin))
                return false;

            manifold.Normal = -manifold.Normal;
            return true;
        }

        manifold.PointCount = 0;

        switch (a.Type)
        {
        case eXcolliderShape::Sphere:
            switch (b.Type)
            {
            case eXcolliderShape::Sphere:
                return SphereSphere(a, b, margin, manifold);
            case eXcolliderShape::Box:
                return SphereBox(a, b, margin, manifold);
            case eXcolliderShape::Plane:
                return SpherePlane(a, b, margin, manifold);
            }
            break;
        case eXcolliderShape::Box:
            if (b.Type == eXcolliderShape::Box)
                return BoxBox(a, b, margin, manifold);
            if (b.Type == eXcolliderShape::Plane)
                return BoxPlane(a, b, margin, manifold);
            break;
        case eXcolliderShape::Plane:
            break;
        }

        return false;
    }

    EXBOOL Raycast(const eXshape &shape, const EXMATH::vec3 &origin, const EXMATH::vec3 &direction,
                   EXFLOAT maxDistance, EXFLOAT &distance, EXMATH::vec3 &normal)
    {
        switch (shape.Type)
        {
        case eXcolliderShape::Sphere:
        {
            const vec3 m = origin - shape.Center;
            const EXFLOAT b = EXMATH::dot(m, direction);
            const EXFLOAT c = EXMATH::dot(m, m) - shape.Radius * shape.Radius;

            // Outside and pointing away.
            if (c > 0.0f && b > 0.0f)
                return false;

            const EXFLOAT discriminant = b * b - c;

            if (discriminant < 0.0f)
                return false;

            distance = std::max(-b - std::sqrt(discriminant), 0.0f);
            normal = distance > 0.0f ? EXMATH::normalize(origin + direction * distance - shape.Center) : -direction;
            break;
        }
        case eXcolliderShape::Box:
        {
            // Slab test in the box's space: the ray is inside between entering and leaving
            // every pair of parallel faces.
            const EXMATH::mat3 toLocal = EXMATH::transpose(shape.Axes);
            const vec3 localOrigin = toLocal * (origin - shape.Center);
            const vec3 localDirection = toLocal * direction;

            EXFLOAT enter = -FLT_MAX, leave = FLT_MAX;
            EXINT enterAxis = 0;
            EXFLOAT enterSign = 1.0f;

            for (EXINT k = 0; k < 3; ++k)
            {
                if (std::abs(localDirection[k]) < 1e-8f)
                {
                    if (std::abs(localOrigin[k]) > shape.HalfExtents[k])
                        return false;

                    continue;
                }

                EXFLOAT entering = (-shape.HalfExtents[k] - localOrigin[k]) / localDirection[k];
                EXFLOAT leaving = (shape.HalfExtents[k] - localOrigin[k]) / localDirection[k];
                EXFLOAT sign = -1.0f;

                if (entering > leaving)
                {
                    std::swap(entering, leaving);
                    sign = 1.0f;
                }

                if (entering > enter)
                {
                    enter = entering;
                    enterAxis = k;
                    enterSign = sign;
                }

                leave = std::min(leave, leaving);

                if (enter > leave)
                    return false;
            }

            if (leave < 0.0f)
                return false;

            distance = std::max(enter, 0.0f);
            normal = enter > 0.0f ? shape.Axes[enterAxis] * enterSign : -direction;
            break;
        }
        case eXcolliderShape::Plane:
        {
            const EXFLOAT height = EXMATH::dot(shape.Normal, origin - shape.Center);
            const EXFLOAT speed = EXMATH::dot(shape.Normal, direction);

            // Behind the plane counts as inside it.
            if (height <= 0.0f)
                distance = 0.0f;
            else if (speed >= 0.0f)
                return false;
            else
                distance = -height / speed;

            normal = shape.Normal;
            break;
        }
        }

        return distance <= maxDistance;
    }
}
