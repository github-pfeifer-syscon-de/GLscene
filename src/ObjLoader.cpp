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

#include <iostream>
#include <fstream>
#include <string>
#include <StringUtils.hpp>
#include <GenericGlmCompat.hpp>
#include <limits>
#include <format>
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/glm.hpp> // normalize

#include "ObjLoader.hpp"

#include <Displayable.hpp>

namespace psc::gl {

std::mutex ObjItem::m_MutexObj;

#ifndef CALLBACK
#define CALLBACK
#endif

GLenum ObjTessy::objTess;

static const char*
getPrimitiveTypeObj(GLenum type)
{
    switch(type) {
    case 0x0000:
        return "GL_POINTS";
    case 0x0001:
        return "GL_LINES";
    case 0x0002:
        return "GL_LINE_LOOP";
    case 0x0003:
        return "GL_LINE_STRIP";
    case 0x0004:
        return "GL_TRIANGLES";
    case 0x0005:
        return "GL_TRIANGLE_STRIP";
    case 0x0006:
        return "GL_TRIANGLE_FAN";
    case 0x0007:
        return "GL_QUADS";
    case 0x0008:
        return "GL_QUAD_STRIP";
    case 0x0009:
        return "GL_POLYGON";
    }
    return "?";
}


static void CALLBACK
objBeginCB(GLenum _which)
{
    //std::cout << "objBeginCB() " << getPrimitiveTypeObj(_which) << std::endl;
    ObjTessy::objTess = _which;
}

static void CALLBACK
objEndCB()
{
    //std::cout << "objEndCB();" << std::endl;
}

static void CALLBACK
objErrorCB(GLenum errorCode)
{
   const GLubyte *errorStr = gluErrorString(errorCode);
    std::cerr << "[TESSELATION ERROR]: " << errorStr << std::endl;
}

static void CALLBACK
objVertex(const GLvoid *data)
{
    auto callbackRef = reinterpret_cast<const ObjTessRef*>(data);
    if (callbackRef) {
        callbackRef->callback(ObjTessy::objTess);
    }
    else {
        std::cout << "No callback found !" << std::endl;
    }
}

static void CALLBACK
objCombineCB(const GLdouble newVertex[3], const GLdouble *neighborVertex[4],
            const GLfloat neighborWeight[4], GLdouble **outData)
{
   std::cerr << "Something went wrong ... (check memory allocation for values) tessCombineCB\n"
             << "  new " << newVertex[0] << " " << newVertex[1] << " " << newVertex[2] << "\n"
             << "  weight " << neighborWeight[0] << " " << neighborWeight[1] << " " << neighborWeight[2] << " " << neighborWeight[3] << std::endl;
}

ObjMaterial::ObjMaterial(const std::string& name)
: m_name{name}
{
}

void
ObjMaterial::add(const std::vector<std::string>& parts, std::size_t lineCount)
{
    const auto& type = parts[0];
    if (type == "Ka") { // ambient color
        m_ambientColor = ObjLoader::parse3f(parts, lineCount);
    }
    else if (type == "Kd") { // diffuse color
        m_diffuseColor = ObjLoader::parse3f(parts, lineCount);
    }
    else if (type == "Ks") { // specular color
        m_specularColor = ObjLoader::parse3f(parts, lineCount);
    }
    else if (type == "Ns") { // specular exponent ranges between 0 and 1000
        m_specularExponent = ObjLoader::parse1f(parts, lineCount);
    }

}

Color
ObjMaterial::getAmblientColor()
{
    return m_ambientColor;
}

Color
ObjMaterial::getDiffuseColor()
{
    return m_diffuseColor;
}

ObjItem::ObjItem(const std::string& name, size_t basePos, size_t baseUV, size_t baseNorm)
: m_name{name}
, m_basePos{basePos}
, m_baseUV{baseUV}
, m_baseNorm{baseNorm}
{
    m_pos.reserve(256);
    m_uv.reserve(256);
    m_norm.reserve(256);
    m_vertex.reserve(256);
}

void
ObjItem::addPos(const Position& pos)
{
    m_pos.push_back(pos);
}

void
ObjItem::addNorm(const Vector& norm)
{
    m_norm.push_back(norm);
}

void
ObjItem::addUV(const UV& uv)
{
    m_uv.push_back(uv);
}

void
ObjItem::addVertex(std::vector<ObjIdx>& vertices)
{
    for (auto& idx : vertices) {
        if (idx.pos > 0) {
            idx.pos -= m_basePos;   // only calculate these once make index item local
        }
        else {
            idx.pos = m_pos.size() + idx.pos;   // negative value reverse address, pos -1 -> last element
            // I have no example for this -> test idx is object local
        }
        if (idx.uv > 0) {
            idx.uv -= m_baseUV;
        }
        else {
            if (idx.uv < 0) {
                idx.uv = m_uv.size() + idx.uv;
            }
        }
        if (idx.norm > 0) {
            idx.norm -= m_baseNorm;
        }
        else {
            idx.norm = m_norm.size() + idx.norm;
        }
        if (idx.pos < 0 || idx.pos > static_cast<int32_t>(std::numeric_limits<uint16_t>::max())) {
            std::string msg = std::format("Position index {} below/beyond usable {}"
                , idx.pos, std::numeric_limits<uint16_t>::max());
            throw ObjException(msg);
        }
        if (idx.pos > static_cast<int32_t>(m_pos.size())) {
            auto msg = std::format("Position index {} beyond position values {}"
                    , idx.pos, m_pos.size());
            throw ObjException(msg);
        }
        if (idx.uv != 0) {  // use 0 as n/v value
            if (idx.uv < 0 || idx.uv > static_cast<int32_t>(std::numeric_limits<uint16_t>::max())) {
                auto msg = std::format("Texture index {} below/beyond usable {}"
                        , idx.uv, std::numeric_limits<uint16_t>::max());
                throw ObjException(msg);
            }
            if (idx.uv > static_cast<int32_t>(m_uv.size())) {
                auto msg = std::format("Texture index {} beyond texture values {}"
                        , idx.uv, m_uv.size());
                throw ObjException(msg);
            }
        }
        // we expect normals even if the obj spec does not require it
        if (idx.norm < 0 || idx.norm > static_cast<int32_t>(std::numeric_limits<uint16_t>::max())) {
            auto msg = std::format( "Normal index {} below/beyond usable {}"
                    , idx.norm, std::numeric_limits<uint16_t>::max());
            throw ObjException(msg);
        }
        if (idx.norm > static_cast<int32_t>(m_norm.size())) {
            auto msg = std::format("Position index {} beyond normal values {}"
                , idx.norm, m_norm.size());
            throw ObjException(msg);
        }

    }
    m_vertex.push_back(vertices);
}

size_t
ObjItem::getPosBase()
{
    return m_basePos + m_pos.size();
}

size_t
ObjItem::getNormBase()
{
    return m_baseNorm + m_norm.size();
}

size_t
ObjItem::getUVBase()
{
    return m_baseUV + m_uv.size();
}

void
ObjItem::setActiveMaterial(const PtrObjMaterial& activeMaterial)
{
    m_activeMaterial = activeMaterial;
}

// use indexed geometry
void
ObjItem::addVertex(const ObjIdx& objIdx, psc::mem::active_lease<psc::gl::Geom2>& lgeo)
{
    // each value will use only 16Bit, presume constant color
    uint32_t idx = objIdx.pos << 16 | objIdx.norm;
    auto pos = m_usedIndexes.find(idx);
    uint32_t geoIdx;
    if (pos == m_usedIndexes.end()) {
        size_t idxPos = objIdx.pos;
        const auto& pos = m_pos[idxPos];
        size_t idxNorm = objIdx.norm;
        auto objNormal = m_norm[idxNorm];
        objNormal.y *= -1.0f;    // this brightens our picture
        Color c{0.5f, 0.5f, 0.5f};
        if (m_activeMaterial) {
            c = m_activeMaterial->getDiffuseColor();
        }
        geoIdx = lgeo->getVertexIndex();
        lgeo->addPoint(&pos, &c, &objNormal);
        m_usedIndexes.insert(std::pair(idx, geoIdx));
        ++m_indexed;
    }
    else {
        geoIdx = pos->second;
    }
    ++m_added;
    lgeo->addIndex(geoIdx);
}

void
ObjItem::tessCallback(size_t index, GLenum objTess)
{
    //std::cout << "ObjItem::tessCallback" << m_vertexIdx << " "  << index << std::endl;
    const auto& indexes = m_vertex[m_vertexIdx];
    auto& objIdx = indexes[index];
    if (auto lgeo = m_geom.lease()) {
        switch(objTess) {
        case GL_TRIANGLES :
            //std::cout << "Tess tri " << pos.x << " " << pos.y << " " << pos.z << std::endl;
            addVertex(objIdx, lgeo);
            break;
        case GL_TRIANGLE_FAN:   // to use uniform geometry convert to triangles
            //std::cout << "Tess triFan " << pos.x << " " << pos.y << " " << pos.z << std::endl;
            if (m_objLastIndex < 2) {
                m_idxLast[m_objLastIndex] = objIdx;
                ++m_objLastIndex;
            }
            else {
                addVertex(m_idxLast[0], lgeo);
                addVertex(m_idxLast[1], lgeo);
                addVertex(objIdx, lgeo);
                // with a fan point 0 stays
                m_idxLast[1] = objIdx;
            }
            break;
        case GL_TRIANGLE_STRIP:   // to use uniform geometry convert to triangles
            //std::cout << "Tess triStr " << pos.x << " " << pos.y << " " << pos.z << std::endl;
            if (m_objLastIndex < 2) {
                m_idxLast[m_objLastIndex] = objIdx;
                ++m_objLastIndex;
            }
            else {
                addVertex(m_idxLast[0], lgeo);
                addVertex(m_idxLast[1], lgeo);
                addVertex(objIdx, lgeo);
                m_idxLast[0] = m_idxLast[1];
                m_idxLast[1] = objIdx;
            }
            break;
        default:
            std::cerr << "Undefined " << getPrimitiveTypeObj(objTess) << std::endl;
        }
    }
    // DEBUG //
    //std::cout << "  glVertex3d(" << *ptr << ", " << *(ptr+1) << ", " << *(ptr+2) << ");" << std::endl;
}


void
ObjItem::tesselateGlu()
{
    ObjTessy objTessy(this);  // since we are not completely sure about the configuration of polygons use Glu-tesselator
    for (size_t i = 0; i < m_vertex.size(); ++i) {
        m_vertexIdx = i;
        const auto& indexes = m_vertex[i];
        //std::cout << "ObjItem new vertex  " << i << "/" << m_vertex.size()
        //          << " indexes " << indexes.size() << std::endl;
        if (indexes.size() > 1) {
            [[maybe_unused]] auto norm = objTessy.calculateNormal(indexes, m_pos);
            //std::cout << "Norm " << " " <<  norm.x << " " << norm.y << " " << norm.z << std::endl;
            m_objLastIndex = 0;
            objTessy.beginPolygon(indexes.size());
            objTessy.beginContour();
            for (size_t j = 0; j < indexes.size(); ++j) {
                auto& objIdx = indexes[j];
                auto& itemPos = m_pos[objIdx.pos];
                //std::cout << "Pos " << j << " " <<  itemPos[0] << " " << itemPos[1] << " " << itemPos[2] << std::endl;
                objTessy.vertex(itemPos, j);
            }
            objTessy.endContour();  // vertex will be closed by default
            objTessy.endPolygon();
        }
    }
    std::cout << "ObjItem"
              << " indexed " << m_indexed
              << " added " << m_added << std::endl;
}

// simple version, even when leaving some the 5er+ vertexs this looks plausible ...
//  use -> e.g. export triangulated -> files will be substantially larger
void
ObjItem::tesselate()
{
    if (auto lgeo = m_geom.lease()) {
        for (size_t i = 0; i < m_vertex.size(); ++i) {
            const auto& indexes = m_vertex[i];
            std::cout << "Obj new vertex  " << i << "/" << m_vertex.size() << std::endl;

            if (indexes.size() >= 3) {
                addVertex(indexes[0], lgeo);
                addVertex(indexes[1], lgeo);
                addVertex(indexes[2], lgeo);
            }
            //if (indexes.size() >= 4) {  // the following are wild guesses ...
            //    addVertex(indexes[0], lgeo);
            //    addVertex(indexes[2], lgeo);
            //    addVertex(indexes[3], lgeo);
            //}
            //if (indexes.size() >= 5) {
            //    addVertex(indexes[0], lgeo);
            //    addVertex(indexes[3], lgeo);
            //    addVertex(indexes[4], lgeo);
            //}
        }
    }
}

void
ObjItem::triangulate(GeometryContext *ctx)
{
    if (!m_geom) {
        std::lock_guard<std::mutex> lock(m_MutexObj); // this allows only one instance at a time
        m_geom = psc::mem::make_active<psc::gl::Geom2>(GL_TRIANGLES, ctx);
        //tesselate();
        tesselateGlu();
    }
}

psc::gl::aptrGeom2
ObjItem::getGeometry(GeometryContext *ctx)
{
    if (!m_geom) {
        triangulate(ctx);
    }
    if (auto lgeo = m_geom.lease()) {
        if (lgeo->getNumVertex() == 0) {
            lgeo->create_vao();
        }
    }
    return m_geom;
}

std::string
ObjItem::getInfo()
{
    return std::format("Item {} pos {} norm {} uv {}"
                        , m_name, m_pos.size(), m_norm.size(), m_uv.size());
}

ObjTessRef::ObjTessRef(ObjTessCallback* tessCallback, size_t ctx)
: m_tessCallback{tessCallback}
, m_ctx{ctx}
{
}

void
ObjTessRef::callback(GLenum objTess) const
{
    if (m_tessCallback) {
        m_tessCallback->tessCallback(m_ctx, objTess);
    }
    else {
        std::cout << "ObjTessRef::callback no reference " << m_tessCallback << std::endl;
    }
}

ObjTessy::ObjTessy(ObjTessCallback* tessCallback/*, size_t vertexCnt*/)
: m_tessCallback{tessCallback}
, m_tess{gluNewTess()}
{
    //m_values.reserve(vertexCnt * 3u);   // as we need 3 coords per vertex
    gluTessCallback(m_tess, GLU_TESS_BEGIN, (void (CALLBACK *)())objBeginCB);
    gluTessCallback(m_tess, GLU_TESS_END, (void (CALLBACK *)())objEndCB);
    gluTessCallback(m_tess, GLU_TESS_ERROR, (void (CALLBACK *)())objErrorCB);
    gluTessCallback(m_tess, GLU_TESS_VERTEX, (void (CALLBACK *)())objVertex);
    // If your primitive is self-intersecting, you must also specify a callback to create new vertices:
    gluTessCallback(m_tess, GLU_TESS_COMBINE, (void (CALLBACK *)())objCombineCB);
    gluTessProperty(m_tess, GLU_TESS_WINDING_RULE, GLU_TESS_WINDING_POSITIVE); // POSITIVE dont expect nested  NONZERO
}

ObjTessy::~ObjTessy()
{
    if (m_tess) {
        gluDeleteTess(m_tess);
        m_tess = nullptr;
    }
}

// see https://wikis.khronos.org/opengl/Calculating_a_Surface_Normal
//   Newell's method
// guess: calculate a surface normal improves this as the quality of actual data may vary
//   and this not just messes up lighting (blender, requires sometimes a calculation step)
Vector
ObjTessy::calculateNormal(const std::vector<ObjIdx>& polygon, const std::vector<Position>& pos)
{
    Vector norm{};
    for (size_t i = 0; i < polygon.size(); i++) {
        auto& objIdx = polygon[i];
        size_t idxPos = objIdx.pos;
        const auto& vertCurr = pos[idxPos];
        auto& objNext = polygon[(i + 1) % polygon.size()];
        size_t idxNext = objNext.pos;
        const auto& vertNext = pos[idxNext];

        // Set Normal.x to Sum of Normal.x and (multiply (Current.y minus Next.y) by (Current.z plus Next.z))
        norm.x += (vertCurr.y - vertNext.y) * (vertCurr.z + vertNext.z);
        // Set Normal.y to Sum of Normal.y and (multiply (Current.z minus Next.z) by (Current.x plus Next.x))
        norm.y += (vertCurr.z - vertNext.z) * (vertCurr.x + vertNext.x);
        // Set Normal.z to Sum of Normal.z and (multiply (Current.x minus Next.x) by (Current.y plus Next.y))
        norm.z += (vertCurr.x - vertNext.x) * (vertCurr.y + vertNext.y);
    }
    norm = glm::normalize(norm);
    normal(norm);
    return norm;
}

void
ObjTessy::normal(const Vector& norm)
{
    gluTessNormal(m_tess, norm.x, norm.y, norm.z);
}

void
ObjTessy::beginPolygon(size_t vertexCnt)
{
    m_values.reserve(vertexCnt * 3u);
    m_callback_references.reserve(vertexCnt);
    gluTessBeginPolygon(m_tess, nullptr);
}

void
ObjTessy::endPolygon()
{
    gluTessEndPolygon(m_tess);
    m_values.clear();
    m_callback_references.clear();
}

void
ObjTessy::beginContour()
{
    gluTessBeginContour(m_tess);
}

void
ObjTessy::endContour()
{
    gluTessEndContour(m_tess);
}

void
ObjTessy::vertex(const Position &pos, size_t ctx)
{
    if (m_values.size() + 3u > m_values.capacity()) {
        auto msg = std::format("The polygon was initalized for {} but now more are added, this breaks the tesselator references.", m_values.capacity() / 3u);
        throw ObjException(msg);
    }
    auto dpos = &*m_values.end();
    m_values.push_back(pos.x);  // make values persistent
    m_values.push_back(pos.y);
    m_values.push_back(pos.z);
    auto ref = &*m_callback_references.end();
    m_callback_references.emplace_back(std::move(ObjTessRef(m_tessCallback, ctx)));
    gluTessVertex(m_tess, dpos, reinterpret_cast<void *>(ref));
}

ObjLoader::ObjLoader()
{
    m_items.reserve(8);
}

size_t
ObjLoader::getItems()
{
    return m_items.size();
}

PtrObjItem
ObjLoader::getItem(int idx)
{
    return m_items[idx];
}

PtrObjItem
ObjLoader::addObject(const std::string& oname)
{
    std::string name;
    if (!oname.empty()) {
        name = oname;
    }
    else {
        name = std::format("obj.{}", m_items.size());
    }
    size_t base_pos{1};     // use 1 as the positive indexes start with 1
    size_t base_norm{1};
    size_t base_uv{1};
    if (m_items.size() > 0) {
        auto last = *m_items.rbegin();
        base_pos = last->getPosBase();
        base_norm = last->getNormBase();
        base_uv = last->getUVBase();
    }
    auto obj = std::make_shared<ObjItem>(name, base_pos, base_uv, base_norm);
    m_items.push_back(obj);
    return obj;
}

void
ObjLoader::load(const Glib::RefPtr<Gio::File>& objFile)
{
    std::ifstream stream;
    std::string line;

    std::ios_base::iostate exceptionMask = stream.exceptions() | std::ios::failbit | std::ios::badbit;
    stream.exceptions(exceptionMask);
    try {
        stream.open(objFile->get_path());
        PtrObjItem obj;
        std::size_t lineCount{};
        while (!stream.eof()) {
            std::getline(stream, line);
            StringUtils::trim(line);
            if (line.length() > 0 && line[0] != '#') {
                auto parts = StringUtils::splitConsec(line, ' ');
                if (!parts.empty()) {
                    const auto& type = parts[0];
                    if (type == "o") {  // object (optionally)
                        obj = addObject(parts.size() > 1 ? parts[1] : "");
                    }
                    else if (type == "v") {    // vertex
                        if (!obj) {
                            obj = addObject("");
                        }
                        auto vertex = parse3f(parts, lineCount);
                        obj->addPos(vertex);
                    }
                    else if (type == "vn") {    // normal
                        if (!obj) {
                            auto msg = std::format("No object while loading normals, line {}", line);
                            throw ObjException(msg);
                        }
                        auto norm = parse3f(parts, lineCount);
                        norm = glm::normalize(norm);
                        obj->addNorm(norm);
                    }
                    else if (type == "vt") {    // texture
                        if (!obj) {
                            auto msg = std::format("No object while loading texture coordiantes, line {}", lineCount);
                            throw ObjException(msg);
                        }
                        auto uv = parse2f(parts, lineCount);
                        obj->addUV(uv);
                    }
                    else if (type == "f") {
                        if (!obj) {
                            auto msg = std::format("No object while loading vertex index, line {}", lineCount);
                            throw ObjException(msg);
                        }
                        std::vector<ObjIdx> vertices;
                        vertices.reserve(parts.size() - 1);
                        for (size_t i = 1; i < parts.size(); ++i) {
                            auto idx = parseIdx(parts[i], lineCount);
                            vertices.push_back(idx);
                        }
                        obj->addVertex(vertices);
                    }
                    else if (type == "mtllib") {
                        if (parts.size() > 1) {
                            auto file = objFile->get_parent()->get_child(parts[1]);
                            loadMaterial(file);
                        }
                        else {
                            std::cout << "The expected material file was not given" << std::endl;
                        }
                    }
                    else if (type == "usemtl") {
                        if (parts.size() > 1) {
                            auto materialIter = m_materials.find(parts[1]);
                            if (materialIter != m_materials.end()) {
                                if (!obj) {
                                    auto msg = std::format("No object while assigning material, line {}", lineCount);
                                    throw ObjException(msg);
                                }
                                obj->setActiveMaterial(materialIter->second);
                            }
                            else {
                                auto msg = std::format("The material name {} was not found, line {}", parts[1], lineCount);
                                throw ObjException(msg);
                            }
                        }
                        else {
                            std::cout << "The expected material name was not given" << std::endl;
                        }
                    }
                    else if (type == "s") { // smooth seems rather informative so not complain about
                    }
                    else {
                        auto msg = std::format("ObjLoader::load unknown type {}", line);
                        std::cout << msg << std::endl;
                    }
                }
            }
            ++lineCount;
        }
        stream.close();
    }
    catch (const std::ios_base::failure& e) {
        if (!stream.eof()) {
            auto error = std::format("Error {} reading {}"
                , e.what(), objFile ? objFile->get_path() : "null");
            throw ObjException(error);
        }
    }
}

void
ObjLoader::loadMaterial(const Glib::RefPtr<Gio::File>& materialFile)
{
    std::ifstream stream;

    std::ios_base::iostate exceptionMask = stream.exceptions() | std::ios::failbit | std::ios::badbit;
    stream.exceptions(exceptionMask);
    try {
        stream.open(materialFile->get_path());
        PtrObjItem obj;
        std::string line;
        std::size_t lineCount{};
        PtrObjMaterial activeMaterial;
        while (!stream.eof()) {
            std::getline(stream, line);
            StringUtils::trim(line);
            if (line.length() > 0) {
                if (line[0] == '#') {
                    continue;
                }
                auto parts = StringUtils::splitConsec(line, ' ');
                if (!parts.empty()) {
                    const auto& type = parts[0];
                    if (type == "newmtl") {
                        if (parts.size() > 1) {
                            activeMaterial = std::make_shared<ObjMaterial>(parts[1]);
                            m_materials.insert(std::pair(parts[1], activeMaterial));
                        }
                        else {
                            std::cout << "Missing name for newmtl reading material" << std::endl;
                        }
                    }
                    else {
                        if (activeMaterial) {
                            activeMaterial->add(parts, lineCount);
                        }
                        else {
                            std::cout << "No material to add " << line << std::endl;
                        }
                    }
                }
            }
            else {
                activeMaterial.reset(); // reset active material on empty line
            }
            ++lineCount;
        }
    }
    catch (const std::ios_base::failure& e) {
        if (!stream.eof()) {
            auto error = std::format("Error {} material file {}"
                , e.what(), materialFile ? materialFile->get_path() : "null");
            throw ObjException(error);
        }
    }
}

GLfloat
ObjLoader::parse1f(const std::vector<std::string>& parts, std::size_t lineCount)
{
    GLfloat ret{};
    if (parts.size() > 1) {
        try {
            ret = std::stof(parts[1]);
        }
        catch (const std::exception& e) {
            auto msg = std::format("Error parsing single float {} line {}"
                    , parts[1], lineCount);
            throw ObjException(msg);
        }
    }
    else {
        auto msg = std::format( "Expecting at least 1 got {} for single float {}"
                , parts.size(),  lineCount);
        throw ObjException(msg);
    }
    return ret;
}

glm::vec3
ObjLoader::parse3f(const std::vector<std::string>& parts, std::size_t lineCount)
{
    glm::vec3 ret{};
    if (parts.size() > 3) {
        try {
            ret.x = std::stof(parts[1]);
            ret.y = std::stof(parts[2]);
            ret.z = std::stof(parts[3]);
            if (parts.size() > 4) {  // we have a w
                float w = std::stof(parts[4]);
                if (w != 0.0) {
                    ret.x /= w;
                    ret.y /= w;
                    ret.z /= w;
                }
            }
        }
        catch (const std::exception& e) {
            auto msg = std::format("Error parsing position/normal/color number {} {} {} line {}"
                    , parts[1], parts[2], parts[3], lineCount);
            throw ObjException(msg);
        }
    }
    else {
        auto msg = std::format("Expecting at least 3 got {} position/normals/color line {}"
                , parts.size() - 1, lineCount);
        throw ObjException(msg);
    }
    return ret;
}

glm::vec2
ObjLoader::parse2f(const std::vector<std::string>& parts, std::size_t lineCount)
{
    glm::vec2 ret{};
    if (parts.size() > 2) {
        try {
            ret.x = std::stof(parts[1]);
            ret.y = std::stof(parts[2]);
        }
        catch (const std::exception& e) {
            auto msg = std::format("Error parsing texture number {} {} line {}"
                    , parts[1], parts[2], lineCount);
            throw ObjException(msg);
        }
    }
    else {
        auto msg = std::format( "Expecting at least 2 got {} vertices line {} "
                , parts.size() - 1,  lineCount );
        throw ObjException(msg);
    }
    return ret;
}


ObjIdx
ObjLoader::parseIdx(const std::string& part, std::size_t lineCount)
{
    ObjIdx objIdx{};
    auto parts = StringUtils::splitEach(part, '/');
    if (parts.size() == 3) {    // we expect normals even if the obj spec does not require it
        try {
            objIdx.pos = std::stoi(parts[0]);
            if (!parts[1].empty()) {
                objIdx.uv = std::stoi(parts[1]);
            }
            objIdx.norm = std::stoi(parts[2]);
        }
        catch (const std::exception& e) {
            auto msg = std::format("Error parsing index numbers {} line {}"
                    , part, lineCount);
            throw ObjException(msg);
        }
    }
    else {
        auto msg = std::format( "Expecting at least 3 got {} indexes line {}"
                , parts.size(),  lineCount);
        throw ObjException(msg);
    }
    return objIdx;
}

} /* namespace psc::gl */
