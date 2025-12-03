/**
 * Frustum.cpp - View Frustum Implementation
 */

#include "Frustum.h"  // Adjust path as needed for your project structure
#include <cmath>
#include <algorithm>

// ============================================================================
// PLANE IMPLEMENTATION
// ============================================================================

Plane::Plane(const glm::vec3& point, const glm::vec3& norm)
    : normal{glm::normalize(norm)}
    , distance{glm::dot(normal, point)}
{
}

Plane::Plane(float a, float b, float c, float d)
    : normal{a, b, c}
    , distance{-d}  // Standard form has Ax + By + Cz + D = 0, we store -D
{
    normalize();
}

float Plane::signedDistanceTo(const glm::vec3& point) const noexcept {
    return glm::dot(normal, point) - distance;
}

void Plane::normalize() {
    float length = glm::length(normal);
    if (length > 0.0001f) {
        normal /= length;
        distance /= length;
    }
}

// ============================================================================
// FRUSTUM - Matrix Extraction Method (Gribb/Hartmann)
// ============================================================================

void Frustum::updateFromViewProjection(const glm::mat4& vp) {
    /*
     * GRIBB/HARTMANN PLANE EXTRACTION
     * ================================
     * Reference: "Fast Extraction of Viewing Frustum Planes from the
     *            World-View-Projection Matrix" by Gil Gribb & Klaus Hartmann
     *
     * For a point P, after transformation: P' = VP * P
     * The point is inside the frustum if: -w' <= x', y', z' <= w'
     *
     * This gives us 6 inequalities, each defining a plane:
     *   Left:   w' + x' >= 0  →  (row3 + row0) · P >= 0
     *   Right:  w' - x' >= 0  →  (row3 - row0) · P >= 0
     *   Bottom: w' + y' >= 0  →  (row3 + row1) · P >= 0
     *   Top:    w' - y' >= 0  →  (row3 - row1) · P >= 0
     *   Near:   w' + z' >= 0  →  (row3 + row2) · P >= 0
     *   Far:    w' - z' >= 0  →  (row3 - row2) · P >= 0
     *
     * GLM uses column-major storage, so:
     *   vp[col][row]
     *   row0 = vp[0][0], vp[1][0], vp[2][0], vp[3][0]
     *   row3 = vp[0][3], vp[1][3], vp[2][3], vp[3][3]
     */

    // Left: row3 + row0
    m_planes[Left].normal.x = vp[0][3] + vp[0][0];
    m_planes[Left].normal.y = vp[1][3] + vp[1][0];
    m_planes[Left].normal.z = vp[2][3] + vp[2][0];
    m_planes[Left].distance = -(vp[3][3] + vp[3][0]);
    m_planes[Left].normalize();

    // Right: row3 - row0
    m_planes[Right].normal.x = vp[0][3] - vp[0][0];
    m_planes[Right].normal.y = vp[1][3] - vp[1][0];
    m_planes[Right].normal.z = vp[2][3] - vp[2][0];
    m_planes[Right].distance = -(vp[3][3] - vp[3][0]);
    m_planes[Right].normalize();

    // Bottom: row3 + row1
    m_planes[Bottom].normal.x = vp[0][3] + vp[0][1];
    m_planes[Bottom].normal.y = vp[1][3] + vp[1][1];
    m_planes[Bottom].normal.z = vp[2][3] + vp[2][1];
    m_planes[Bottom].distance = -(vp[3][3] + vp[3][1]);
    m_planes[Bottom].normalize();

    // Top: row3 - row1
    m_planes[Top].normal.x = vp[0][3] - vp[0][1];
    m_planes[Top].normal.y = vp[1][3] - vp[1][1];
    m_planes[Top].normal.z = vp[2][3] - vp[2][1];
    m_planes[Top].distance = -(vp[3][3] - vp[3][1]);
    m_planes[Top].normalize();

    // Near: row3 + row2
    m_planes[Near].normal.x = vp[0][3] + vp[0][2];
    m_planes[Near].normal.y = vp[1][3] + vp[1][2];
    m_planes[Near].normal.z = vp[2][3] + vp[2][2];
    m_planes[Near].distance = -(vp[3][3] + vp[3][2]);
    m_planes[Near].normalize();

    // Far: row3 - row2
    m_planes[Far].normal.x = vp[0][3] - vp[0][2];
    m_planes[Far].normal.y = vp[1][3] - vp[1][2];
    m_planes[Far].normal.z = vp[2][3] - vp[2][2];
    m_planes[Far].distance = -(vp[3][3] - vp[3][2]);
    m_planes[Far].normalize();
}

// ============================================================================
// FRUSTUM - Geometric Construction Method
// ============================================================================

void Frustum::updateFromCamera(const glm::vec3& position,
                                const glm::vec3& front,
                                const glm::vec3& up,
                                const glm::vec3& right,
                                float fovY,
                                float aspect,
                                float nearDist,
                                float farDist)
{
    /*
     * GEOMETRIC FRUSTUM CONSTRUCTION
     * ==============================
     * Build planes from camera vectors and FOV parameters.
     *
     * We calculate points on the far plane, then create planes
     * from the camera position through those points.
     */

    // Half-dimensions at the far plane
    const float halfVSide = farDist * std::tan(fovY * 0.5f);
    const float halfHSide = halfVSide * aspect;

    // Vector to center of far plane
    const glm::vec3 toFarCenter = farDist * front;

    // Near and far planes
    m_planes[Near] = Plane{position + nearDist * front, front};
    m_planes[Far]  = Plane{position + toFarCenter, -front};

    // Side planes (use cross products to find normals)
    m_planes[Right] = Plane{
        position,
        glm::cross(toFarCenter - right * halfHSide, up)
    };

    m_planes[Left] = Plane{
        position,
        glm::cross(up, toFarCenter + right * halfHSide)
    };

    m_planes[Top] = Plane{
        position,
        glm::cross(right, toFarCenter - up * halfVSide)
    };

    m_planes[Bottom] = Plane{
        position,
        glm::cross(toFarCenter + up * halfVSide, right)
    };
}

// ============================================================================
// CULLING TESTS
// ============================================================================

bool Frustum::containsPoint(const glm::vec3& point) const noexcept {
    // Point must be in front of all 6 planes
    for (const Plane& plane : m_planes) {
        if (plane.signedDistanceTo(point) < 0.0f) {
            return false;
        }
    }
    return true;
}

bool Frustum::containsSphere(const glm::vec3& center, float radius) const noexcept {
    // Sphere is visible if center is within `radius` of being in front of each plane
    for (const Plane& plane : m_planes) {
        if (plane.signedDistanceTo(center) < -radius) {
            return false;  // Entire sphere is behind this plane
        }
    }
    return true;
}

bool Frustum::containsAABB(const glm::vec3& min, const glm::vec3& max) const noexcept {
    /*
     * AABB-FRUSTUM INTERSECTION TEST
     * ==============================
     * For each plane, we find the "positive vertex" (P-vertex) - the corner
     * of the AABB that is furthest in the direction of the plane normal.
     *
     * If the P-vertex is behind the plane, the entire box is outside.
     *
     * This is more efficient than testing all 8 corners.
     */

    for (const Plane& plane : m_planes) {
        // Find the P-vertex (furthest corner in direction of normal)
        glm::vec3 pVertex;
        pVertex.x = (plane.normal.x >= 0.0f) ? max.x : min.x;
        pVertex.y = (plane.normal.y >= 0.0f) ? max.y : min.y;
        pVertex.z = (plane.normal.z >= 0.0f) ? max.z : min.z;

        // If P-vertex is behind plane, box is completely outside
        if (plane.signedDistanceTo(pVertex) < 0.0f) {
            return false;
        }
    }

    return true;
}

bool Frustum::containsAABBCenterExtents(const glm::vec3& center,
                                         const glm::vec3& extents) const noexcept
{
    /*
     * AABB test using center + extents form.
     *
     * For each plane, we project the box's extents onto the plane normal
     * to find the "effective radius" - how thick the box appears from
     * that direction.
     */

    for (const Plane& plane : m_planes) {
        // Project extents onto plane normal
        float effectiveRadius =
            extents.x * std::abs(plane.normal.x) +
            extents.y * std::abs(plane.normal.y) +
            extents.z * std::abs(plane.normal.z);

        // If center is more than effectiveRadius behind plane, box is outside
        if (plane.signedDistanceTo(center) < -effectiveRadius) {
            return false;
        }
    }

    return true;
}