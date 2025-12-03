#pragma once

/**
 * Collision.h - Collision Detection Primitives and Utilities
 *
 * Contains:
 *   - AABB (Axis-Aligned Bounding Box)
 *   - Frustum culling integration
 *   - Collision test functions
 */

#include <glm/glm.hpp>
#include "Engine/Renderer/Frustum.h"

// ============================================================================
// AABB - Axis-Aligned Bounding Box
// ============================================================================

/**
 * Axis-Aligned Bounding Box.
 *
 * A box whose edges are parallel to the world X, Y, Z axes.
 * Used for:
 *   - Player hitbox
 *   - Chunk bounding volumes
 *   - Block collision
 *
 * Stored as min/max corners for easy intersection tests.
 */
struct AABB {
    glm::vec3 min{0.0f};  // Minimum corner (lowest X, Y, Z)
    glm::vec3 max{0.0f};  // Maximum corner (highest X, Y, Z)

    // --- Constructors ---

    AABB() = default;

    /**
     * Construct from min/max corners.
     */
    AABB(const glm::vec3& minCorner, const glm::vec3& maxCorner)
        : min(minCorner), max(maxCorner) {}

    /**
     * Construct from center and half-extents.
     */
    static AABB fromCenterExtents(const glm::vec3& center, const glm::vec3& halfExtents) {
        return AABB{center - halfExtents, center + halfExtents};
    }

    /**
     * Construct from center and individual half-sizes.
     */
    static AABB fromCenterSize(const glm::vec3& center, float halfX, float halfY, float halfZ) {
        return AABB{
            center - glm::vec3{halfX, halfY, halfZ},
            center + glm::vec3{halfX, halfY, halfZ}
        };
    }

    // --- Accessors ---

    /**
     * Get center point of the box.
     */
    [[nodiscard]]
    glm::vec3 getCenter() const {
        return (min + max) * 0.5f;
    }

    /**
     * Get half-extents (distance from center to each face).
     */
    [[nodiscard]]
    glm::vec3 getExtents() const {
        return (max - min) * 0.5f;
    }

    /**
     * Get full size of the box.
     */
    [[nodiscard]]
    glm::vec3 getSize() const {
        return max - min;
    }

    // --- Modifiers ---

    /**
     * Update position from center and half-sizes.
     * This is what PlayerController uses to update the hitbox.
     *
     * @param center  New center position
     * @param halfX   Half-width (X axis)
     * @param halfY   Half-height (Y axis)
     * @param halfZ   Half-depth (Z axis)
     */
    void updatePosition(const glm::vec3& center, float halfX, float halfY, float halfZ) {
        min = center - glm::vec3{halfX, halfY, halfZ};
        max = center + glm::vec3{halfX, halfY, halfZ};
    }

    /**
     * Update position from center and half-extents vector.
     */
    void updatePosition(const glm::vec3& center, const glm::vec3& halfExtents) {
        min = center - halfExtents;
        max = center + halfExtents;
    }

    /**
     * Translate the box by an offset.
     */
    void translate(const glm::vec3& offset) {
        min += offset;
        max += offset;
    }

    /**
     * Expand the box to include a point.
     */
    void expandToInclude(const glm::vec3& point) {
        min = glm::min(min, point);
        max = glm::max(max, point);
    }

    /**
     * Expand the box by a margin in all directions.
     */
    void expand(float margin) {
        min -= glm::vec3{margin};
        max += glm::vec3{margin};
    }

    // --- Queries ---

    /**
     * Check if a point is inside the box.
     */
    [[nodiscard]]
    bool containsPoint(const glm::vec3& point) const {
        return point.x >= min.x && point.x <= max.x &&
               point.y >= min.y && point.y <= max.y &&
               point.z >= min.z && point.z <= max.z;
    }

    /**
     * Check if this box intersects another box.
     */
    [[nodiscard]]
    bool intersects(const AABB& other) const {
        return min.x <= other.max.x && max.x >= other.min.x &&
               min.y <= other.max.y && max.y >= other.min.y &&
               min.z <= other.max.z && max.z >= other.min.z;
    }
};

// ============================================================================
// COLLISION NAMESPACE - Free Functions for Collision Tests
// ============================================================================

namespace Collision {

/**
 * Test if two AABBs intersect.
 */
[[nodiscard]]
inline bool AABBIntersectsAABB(const AABB& a, const AABB& b) {
    return a.intersects(b);
}

[[nodiscard]]bool AABBIntersectsFrustum(const AABB& aabb, const Frustum& frustum);
/**
 * Test if a sphere intersects a frustum.
 *
 * @param center   Sphere center
 * @param radius   Sphere radius
 * @param frustum  The view frustum
 */
[[nodiscard]]
inline bool SphereIntersectsFrustum(const glm::vec3& center, float radius, const Frustum& frustum) {
    return frustum.containsSphere(center, radius);
}

/**
 * Test if a point is inside a frustum.
 */
[[nodiscard]]
inline bool PointInFrustum(const glm::vec3& point, const Frustum& frustum) {
    return frustum.containsPoint(point);
}

/**
 * Test if a point is inside an AABB.
 */
[[nodiscard]]
inline bool PointInAABB(const glm::vec3& point, const AABB& aabb) {
    return aabb.containsPoint(point);
}

/**
 * Calculate the penetration depth between two intersecting AABBs.
 * Returns zero vector if not intersecting.
 *
 * The returned vector points from 'a' toward 'b' and represents
 * the minimum translation to separate the boxes.
 */
[[nodiscard]]
inline glm::vec3 AABBPenetration(const AABB& a, const AABB& b) {
    // Check for intersection first
    if (!a.intersects(b)) {
        return glm::vec3{0.0f};
    }

    // Calculate overlap on each axis
    float overlapX = std::min(a.max.x - b.min.x, b.max.x - a.min.x);
    float overlapY = std::min(a.max.y - b.min.y, b.max.y - a.min.y);
    float overlapZ = std::min(a.max.z - b.min.z, b.max.z - a.min.z);

    // Find minimum overlap axis
    glm::vec3 penetration{0.0f};

    if (overlapX <= overlapY && overlapX <= overlapZ) {
        // X axis has smallest overlap
        penetration.x = (a.getCenter().x < b.getCenter().x) ? -overlapX : overlapX;
    } else if (overlapY <= overlapZ) {
        // Y axis has smallest overlap
        penetration.y = (a.getCenter().y < b.getCenter().y) ? -overlapY : overlapY;
    } else {
        // Z axis has smallest overlap
        penetration.z = (a.getCenter().z < b.getCenter().z) ? -overlapZ : overlapZ;
    }

    return penetration;
}

} // namespace Collision