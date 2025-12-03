#define GLM_ENABLE_EXPERIMENTAL
#include "Engine/Physics/Collision.h"
#include <iostream> // For debug output

namespace Collision {

    // Helper function to get the positive vertex (P-vertex) of an AABB relative to a plane normal
    glm::vec3 getPVertex(const AABB& aabb, const glm::vec3& normal) {
        glm::vec3 pVertex = aabb.min;
        if (normal.x >= 0) pVertex.x = aabb.max.x;
        if (normal.y >= 0) pVertex.y = aabb.max.y;
        if (normal.z >= 0) pVertex.z = aabb.max.z;
        return pVertex;
    }

    // Helper function to get the negative vertex (N-vertex) of an AABB relative to a plane normal
    glm::vec3 getNVertex(const AABB& aabb, const glm::vec3& normal) {
        glm::vec3 nVertex = aabb.max;
        if (normal.x >= 0) nVertex.x = aabb.min.x;
        if (normal.y >= 0) nVertex.y = aabb.min.y;
        if (normal.z >= 0) nVertex.z = aabb.min.z;
        return nVertex;
    }

    bool AABBIntersectsFrustum(const AABB& aabb, const Frustum& frustum) {
        for (int i = 0; i < 6; ++i) {
            const Plane& plane = frustum.getPlane(static_cast<Frustum::PlaneIndex>(i));

            // Use the P-vertex (furthest in the direction of the plane normal)
            glm::vec3 pVertex = getPVertex(aabb, plane.normal);
            float signedDistance = plane.signedDistanceTo(pVertex);

            // If even the furthest-in-direction-of-normal corner is behind the plane,
            // the entire AABB is outside.
            if (signedDistance < 0.0f) {
                return false;
            }
        }

        // Otherwise, box is at least partially inside
        return true;
    }

} // namespace Collision
