/* -*- Mode: c++; c-basic-offset: 4; tab-width: 4; coding: utf-8; -*-  */
/*
 * Copyright (C) 2026 RPf
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */
#pragma once

#include <Geom2.hpp>
#include <memory>
#include <mutex>
#include <GL/glu.h>
#include <giomm.h>

namespace psc::gl {
/**
 * This is a incomplete loader, for just a blender model I used
 * Implementation limit:
 *  - indexes in int32_t range
 * from dependency geom2:
 *  - allows only indexes in uint16_t range (not used at the moment)
 **/
struct ObjIdx
{
    int32_t pos;
    int32_t uv;
    int32_t norm;
};

class ObjTessCallback
{
public:
    virtual void tessCallback(size_t ctx, GLenum objTess) = 0;
};

class ObjMaterial
{
public:
    ObjMaterial(const std::string& name);
    explicit ObjMaterial(const ObjMaterial &other) = delete;
    virtual ~ObjMaterial() = default;

    void add(const std::vector<std::string>& parts, std::size_t lineCount);
    Color getAmblientColor();
    Color getDiffuseColor();
private:
    std::string m_name;
    Color m_ambientColor;
    Color m_diffuseColor;
    Color m_specularColor;
    float m_specularExponent;
};

using PtrObjMaterial = std::shared_ptr<ObjMaterial>;


class ObjItem
: public ObjTessCallback
{
public:
    ObjItem(const std::string& name, size_t basePos, size_t baseUV, size_t baseNorm);
    explicit ObjItem(const ObjItem &other) = delete;
    virtual ~ObjItem() = default;

    void addPos(const Position& pos);
    void addNorm(const Vector& norm);
    void addUV(const UV& uv);
    void addVertex(std::vector<ObjIdx>& vertices);
    // reduces the work done when getGeometry will be called
    void triangulate(GeometryContext *m_ctx);
    // this needs to be done with active gl_context
    psc::gl::aptrGeom2 getGeometry(GeometryContext *m_ctx);
    size_t getPosBase();
    size_t getNormBase();
    size_t getUVBase();
    void setActiveMaterial(const PtrObjMaterial& material);

    void tessCallback(size_t ctx, GLenum objTess) override;
    std::string getInfo();
protected:
    void tesselateGlu();
    void tesselate();
    void addVertex(const ObjIdx& objIdx, psc::mem::active_lease<psc::gl::Geom2>& lgeo);

private:
    std::string m_name;
    size_t m_basePos;
    size_t m_baseUV;
    size_t m_baseNorm;
    std::vector<Position> m_pos;
    std::vector<UV> m_uv;
    std::vector<Vector> m_norm;
    std::vector<std::vector<ObjIdx>> m_vertex;
    psc::gl::aptrGeom2 m_geom;
    static std::mutex m_MutexObj;
    size_t m_vertexIdx;
    ObjIdx m_idxLast[2];
    size_t m_objLastIndex;
    PtrObjMaterial m_activeMaterial;
    std::map<uint32_t, uint32_t> m_usedIndexes;
    uint32_t m_added{};
    uint32_t m_indexed{};
};

using PtrObjItem = std::shared_ptr<ObjItem>;

/**
 * serves as reference on callback
 *   static -> object adapter
 **/
struct ObjTessRef
{
    ObjTessRef(ObjTessCallback* tessCallback, size_t ctx);
    virtual ~ObjTessRef() = default;
    void callback(GLenum objTess) const;

    ObjTessCallback* m_tessCallback;
    size_t m_ctx;
};

class ObjTessy
{
public:
    ObjTessy(ObjTessCallback* tessCallback/*, size_t vertexCnt*/);
    explicit ObjTessy(const ObjTessy &other) = delete;
    virtual ~ObjTessy();

    Vector calculateNormal(const std::vector<ObjIdx>& polygon, const std::vector<Position>& pos);
    void normal(const Vector& norm);
    /**
     * the tesselator is sensitive to memory issues
     * (keep the values around while poly is active)
     * -> give a reasonable value for vertexCnt or things may get weired later ...
     **/
    void beginPolygon(size_t vertexCnt);
    void endPolygon();
    void beginContour();
    void endContour();
    void vertex(const Position &pos, size_t ctx);
    static GLenum objTess;

private:
    ObjTessCallback* m_tessCallback;
    GLUtesselator *m_tess;
    std::vector<GLdouble> m_values;
    std::vector<ObjTessRef> m_callback_references;
};

class ObjException
: public std::invalid_argument
{
public:
    ObjException(const std::string& error)
    : std::invalid_argument(error)
    {
    }
};


class ObjLoader
{
public:
    ObjLoader();
    explicit ObjLoader(const ObjLoader &other) = delete;
    virtual ~ObjLoader() = default;

    void load(const Glib::RefPtr<Gio::File>& objFile);
    size_t getItems();
    PtrObjItem getItem(int idx);

    static glm::vec3 parse3f(const std::vector<std::string>& parts, std::size_t lineCount);
    static glm::vec2 parse2f(const std::vector<std::string>& parts, std::size_t lineCount);
    static GLfloat parse1f(const std::vector<std::string>& parts, std::size_t lineCount);

protected:
    void loadMaterial(const Glib::RefPtr<Gio::File>& materialFile);
    PtrObjItem addObject(const std::string& oname);
    ObjIdx parseIdx(const std::string& part, size_t lineCount);
    std::map<std::string, PtrObjMaterial> m_materials;
private:
    std::vector<PtrObjItem> m_items;
    bool m_buildReverseIdx{false};

};
} /* namespace psc::gl */