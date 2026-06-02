#pragma once
#include <string>
#include <memory>
#include <vector>
#include <tuple>
#include "Mesh.hpp"
#include "Material.hpp"
#include "Mat4.hpp"
#include "imgui.h"

namespace core {

    struct {
        std::string type   = "";
        std::string id     = "";
        std::string name   = "";
        std::string color  = "";
        std::string points = "";
    } typedef ObjectDetails;

    enum class ObjectType {
        POINT,
        LINE,
        WIREFRAME,
        NONE,
        POLYGON,
        CURVE2D,
        SURFACE,     // bicubic surface (Bezier or B-Spline), a list of 16-point patches; 3D-only
        MESH,        // imported OBJ mesh: solid, possibly multi-face, full Mesh fidelity
        ENUM_SIZE,
    };

    const char* getTypeName(ObjectType type);

    // Whether a user can create this object type while the app is in the given
    // dimensionality. Point/Line/Wireframe/Polygon work in both modes; Curve2D
    // is 2D-only; Surface is 3D-only. Mesh/None are never user-created here.
    bool typeAvailableInMode(ObjectType type, bool is3d);

    class Object {
    public:
        long long id = 0;
        std::string name;
        ObjectType type = ObjectType::NONE;
        std::shared_ptr<Mesh> mesh;
        Material material;
        mat4 transform;

        Object() : mesh(std::make_shared<Mesh>()), transform(true) {}

        Object(const Object& o)
            : id(o.id), name(o.name), type(o.type),
              mesh(std::make_shared<Mesh>(*o.mesh)),
              material(o.material), transform(o.transform) {}

        Object& operator=(const Object& o) {
            if (this != &o) {
                id = o.id; name = o.name; type = o.type;
                mesh = std::make_shared<Mesh>(*o.mesh);
                material = o.material; transform = o.transform;
            }
            return *this;
        }

        Object(Object&&) = default;
        Object& operator=(Object&&) = default;

        core::Point anchorPoint() const;
        core::Point centerPoint() const;
        ObjectDetails GetObjectDetails() const;
        std::vector<std::tuple<float,float,float>> GetRawPoints() const;
        void ApplyTransformation(const mat4& m);
    };
}
