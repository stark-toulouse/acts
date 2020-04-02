// This file is part of the Acts project.
//
// Copyright (C) 2019 CERN for the benefit of the Acts project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#pragma once

#include "Acts/Utilities/Definitions.hpp"
#include "Acts/Visualization/IVisualization.hpp"

#include <utility>
#include <vector>

namespace Acts {

/// This helper produces output in the OBJ format. Note that colors are not
/// supported in this implementation.
///
template <typename T = double>
class ObjVisualization : public IVisualization {
 public:
  static_assert(std::is_same_v<T, double> || std::is_same_v<T, float>,
                "Use either double or float");

  /// Stored value type, should be double or float
  using ValueType = T;

  /// Type of a vertex based on the value type
  using VertexType = ActsVector<ValueType, 3>;

  /// Type of a line
  using LineType = std::pair<size_t, size_t>;

  /// @copydoc Acts::IVisualization::vertex()
  void vertex(const Vector3D& vtx,
              IVisualization::ColorType color = {0, 0, 0}) override {
    m_vertexColors[m_vertices.size()] = color;
    m_vertices.push_back(vtx.template cast<ValueType>());
  }

  /// @copydoc Acts::IVisualization::line()
  void line(const Vector3D& a, const Vector3D& b,
            IVisualization::ColorType color = {0, 0, 0}) override {
    if (color != IVisualization::ColorType{0, 0, 0}) {
      m_lineColors[m_lines.size()] = color;
    }
    // not implemented
    vertex(a, color);
    vertex(b, color);
    m_lines.push_back({m_vertices.size() - 2, m_vertices.size() - 1});
  }

  /// @copydoc Acts::IVisualization::face()
  void face(const std::vector<Vector3D>& vtxs,
            IVisualization::ColorType color = {0, 0, 0}) override {
    if (color != IVisualization::ColorType{0, 0, 0}) {
      m_faceColors[m_faces.size()] = color;
    }
    FaceType idxs;
    idxs.reserve(vtxs.size());
    for (const auto& vtx : vtxs) {
      vertex(vtx, color);
      idxs.push_back(m_vertices.size() - 1);
    }
    m_faces.push_back(std::move(idxs));
  }

  /// @copydoc Acts::IVisualization::faces()
  void faces(const std::vector<Vector3D>& vtxs,
             const std::vector<FaceType>& faces, ColorType color = {0, 0, 0}) {
    // No faces given - call the face() method
    if (faces.empty()) {
      face(vtxs, color);
    } else {
      if (color != IVisualization::ColorType{0, 0, 0}) {
        m_faceColors[m_faces.size()] = color;
      }
      auto vtxoffs = m_vertices.size();
      if (color != IVisualization::ColorType{0, 0, 0}) {
        m_vertexColors[m_vertices.size()] = color;
      }
      m_vertices.insert(m_vertices.end(), vtxs.begin(), vtxs.end());
      for (const auto& face : faces) {
        if (face.size() == 2) {
          m_lines.push_back({face[0] + vtxoffs, face[2] + vtxoffs});
        } else {
          FaceType rawFace = face;
          std::transform(rawFace.begin(), rawFace.end(), rawFace.begin(),
                         [&](size_t& iv) { return (iv + vtxoffs); });
          m_faces.push_back(rawFace);
        }
      }
    }
  }

  /// @copydoc Acts::IVisualization::write()
  void write(std::ostream& os, std::ostream* eos = nullptr) const final {
    std::map<std::string, bool> materials;

    auto mixColor = [&](const IVisualization::ColorType& color) -> std::string {
      std::string materialName;
      materialName = " material_";
      materialName += std::to_string(color[0]) + std::string("_");
      materialName += std::to_string(color[1]) + std::string("_");
      materialName += std::to_string(color[2]);
      if (eos != nullptr and materials.find(materialName) == materials.end()) {
        (*eos) << "newmtl " << materialName << "\n";
        std::vector<std::string> shadings = {"Ka", "Kd", "Ks"};
        for (const auto& shd : shadings) {
          (*eos) << shd << " " << std::to_string(color[0] / 256.) << " ";
          (*eos) << std::to_string(color[1] / 256.) << " ";
          (*eos) << std::to_string(color[2] / 256.) << " "
                 << "\n";
          (*eos) << "\n";
        }
      }
      return std::string("usemtl ") + materialName;
    };
    size_t iv = 0;
    IVisualization::ColorType lastVertexColor = {0, 0, 0};
    for (const VertexType& vtx : m_vertices) {
      if (m_vertexColors.find(iv) != m_vertexColors.end()) {
        auto color = m_vertexColors.find(iv)->second;
        if (color != lastVertexColor) {
          os << mixColor(color) << "\n";
          lastVertexColor = color;
        }
      }
      os << "v " << vtx.x() << " " << vtx.y() << " " << vtx.z() << "\n";
      ++iv;
    }
    size_t il = 0;
    IVisualization::ColorType lastLineColor = {0, 0, 0};
    for (const LineType& ln : m_lines) {
      if (m_lineColors.find(il) != m_lineColors.end()) {
        auto color = m_lineColors.find(il)->second;
        if (color != lastLineColor) {
          os << mixColor(color) << "\n";
          lastLineColor = color;
        }
      }
      os << "l " << ln.first + 1 << " " << ln.second + 1 << "\n";
      ++il;
    }
    size_t is = 0;
    IVisualization::ColorType lastFaceColor = {0, 0, 0};
    for (const FaceType& fc : m_faces) {
      if (m_faceColors.find(is) != m_faceColors.end()) {
        auto color = m_faceColors.find(is)->second;
        if (color != lastFaceColor) {
          os << mixColor(color) << "\n";
          lastFaceColor = color;
        }
      }
      os << "f";
      for (size_t i = 0; i < fc.size(); i++) {
        os << " " << fc[i] + 1;
      }
      os << "\n";
      ++is;
    }
  }

  ///  @copydoc Acts::IVisualization::clear()
  void clear() override {
    m_vertices.clear();
    m_faces.clear();
    m_lines.clear();
    m_lineColors.clear();
    m_vertexColors.clear();
    m_faceColors.clear();
  }

 private:
  std::vector<VertexType> m_vertices;
  std::vector<FaceType> m_faces;
  std::vector<LineType> m_lines;
  /// Map of colors to be written at given index position
  std::map<size_t, IVisualization::ColorType> m_lineColors;
  std::map<size_t, IVisualization::ColorType> m_vertexColors;
  std::map<size_t, IVisualization::ColorType> m_faceColors;
};
}  // namespace Acts