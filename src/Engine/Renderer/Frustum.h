/**
 * Frustum.h - View Frustum for Culling
 *
 * Integrates with your existing Camera and Collision systems.
 * Used by ChunkManager to cull chunks outside the camera's view.
 */

#pragma once

#include <glm/glm.hpp>
#include <array>

/**
 * A plane in 3D space, defined by the equation:
 *   Ax + By + Cz + D = 0
 *
 * Where (A, B, C) is the normal and D is the distance from origin.
 * Points where Ax + By + Cz + D > 0 are "in front" of the plane.
 */
struct Plane {
    glm::vec3 normal = {0.f, 1.f, 0.f};  // (A, B, C) - must be normalized
    float distance = 0.f;                 // D (negated from standard form)

    Plane() = default;

    /**
     * Construct plane from a point on the plane and its normal.
     */
    Plane(const glm::vec3& point, const glm::vec3& norm);

    /**
     * Construct plane from coefficients (Ax + By + Cz + D = 0).
     * Will normalize automatically.
     */
    Plane(float a, float b, float c, float d);

    /**
     * Signed distance from point to plane.
     *   > 0 : point is in front (same side as normal)
     *   < 0 : point is behind
     *   = 0 : point is on the plane
     */
    [[nodiscard]]
    float signedDistanceTo(const glm::vec3& point) const noexcept;

    /**
     * Normalize the plane (ensure normal has length 1).
     */
    void normalize();
};

/**
 * View Frustum - The 6 planes bounding the camera's visible volume.
 *
 * Used for culling objects (especially chunks) that are outside the view.
 *
 * USAGE IN YOUR ENGINE:
 * ---------------------
 * In Camera::updateFrustum():
 *     m_frustum.updateFromViewProjection(projection * view);
 *
 * In ChunkManager::render():
 *     if (chunk.isInFrustum(camera.getFrustum())) { render(); }
 */
class Frustum {
public:
    /**
     * Plane indices for direct access.
     */
    enum PlaneIndex : size_t {
        Left = 0,
        Right = 1,
        Bottom = 2,
        Top = 3,
        Near = 4,
        Far = 5,
        Count = 6
    };

    Frustum() = default;

    /**
     * Extract frustum planes from a View-Projection matrix.
     *
     * This is the Gribb/Hartmann method - extracts planes directly
     * from the combined matrix. Call this whenever camera moves.
     *
     * @param viewProjection  The combined View * Projection matrix
     *                        (or Projection * View depending on your convention)
     */
    void updateFromViewProjection(const glm::mat4& viewProjection);

    /**
     * Alternative: Build frustum from camera vectors.
     * Use this if you prefer geometric construction over matrix extraction.
     *
     * @param position   Camera position in world space
     * @param front      Camera forward direction (normalized)
     * @param up         Camera up direction (normalized)
     * @param right      Camera right direction (normalized)
     * @param fovY       Vertical field of view in RADIANS
     * @param aspect     Aspect ratio (width / height)
     * @param nearDist   Near plane distance
     * @param farDist    Far plane distance
     */
    void updateFromCamera(const glm::vec3& position,
                          const glm::vec3& front,
                          const glm::vec3& up,
                          const glm::vec3& right,
                          float fovY,
                          float aspect,
                          float nearDist,
                          float farDist);

    // ========================================================================
    // CULLING TESTS - Return true if object is (potentially) visible
    // ========================================================================

    /**
     * Test if a point is inside the frustum.
     */
    [[nodiscard]]
    bool containsPoint(const glm::vec3& point) const noexcept;

    /**
     * Test if a sphere intersects or is inside the frustum.
     *
     * @param center  Sphere center in world space
     * @param radius  Sphere radius
     */
    [[nodiscard]]
    bool containsSphere(const glm::vec3& center, float radius) const noexcept;

    /**
     * Test if an AABB intersects the frustum.
     *
     * This is the primary method for chunk culling!
     *
     * @param min  Minimum corner of the box (world space)
     * @param max  Maximum corner of the box (world space)
     */
    [[nodiscard]]
    bool containsAABB(const glm::vec3& min, const glm::vec3& max) const noexcept;

    /**
     * Test AABB using center + half-extents form.
     *
     * @param center   Center of the box
     * @param extents  Half-size in each dimension
     */
    [[nodiscard]]
    bool containsAABBCenterExtents(const glm::vec3& center,
                                    const glm::vec3& extents) const noexcept;

    // ========================================================================
    // ACCESSORS
    // ========================================================================

    [[nodiscard]] const Plane& getPlane(PlaneIndex index) const noexcept {
        return m_planes[index];
    }

    [[nodiscard]] const Plane& getLeftPlane()   const noexcept { return m_planes[Left]; }
    [[nodiscard]] const Plane& getRightPlane()  const noexcept { return m_planes[Right]; }
    [[nodiscard]] const Plane& getBottomPlane() const noexcept { return m_planes[Bottom]; }
    [[nodiscard]] const Plane& getTopPlane()    const noexcept { return m_planes[Top]; }
    [[nodiscard]] const Plane& getNearPlane()   const noexcept { return m_planes[Near]; }
    [[nodiscard]] const Plane& getFarPlane()    const noexcept { return m_planes[Far]; }

private:
    std::array<Plane, Count> m_planes;
};