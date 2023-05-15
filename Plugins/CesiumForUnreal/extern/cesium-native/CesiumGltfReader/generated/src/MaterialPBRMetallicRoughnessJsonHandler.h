// This file was generated by generate-classes.
// DO NOT EDIT THIS FILE!
#pragma once

#include "TextureInfoJsonHandler.h"

#include <CesiumGltf/MaterialPBRMetallicRoughness.h>
#include <CesiumJsonReader/ArrayJsonHandler.h>
#include <CesiumJsonReader/DoubleJsonHandler.h>
#include <CesiumJsonReader/ExtensibleObjectJsonHandler.h>

namespace CesiumJsonReader {
class ExtensionReaderContext;
}

namespace CesiumGltfReader {
class MaterialPBRMetallicRoughnessJsonHandler
    : public CesiumJsonReader::ExtensibleObjectJsonHandler {
public:
  using ValueType = CesiumGltf::MaterialPBRMetallicRoughness;

  MaterialPBRMetallicRoughnessJsonHandler(
      const CesiumJsonReader::ExtensionReaderContext& context) noexcept;
  void reset(
      IJsonHandler* pParentHandler,
      CesiumGltf::MaterialPBRMetallicRoughness* pObject);

  virtual IJsonHandler* readObjectKey(const std::string_view& str) override;

protected:
  IJsonHandler* readObjectKeyMaterialPBRMetallicRoughness(
      const std::string& objectType,
      const std::string_view& str,
      CesiumGltf::MaterialPBRMetallicRoughness& o);

private:
  CesiumGltf::MaterialPBRMetallicRoughness* _pObject = nullptr;
  CesiumJsonReader::
      ArrayJsonHandler<double, CesiumJsonReader::DoubleJsonHandler>
          _baseColorFactor;
  TextureInfoJsonHandler _baseColorTexture;
  CesiumJsonReader::DoubleJsonHandler _metallicFactor;
  CesiumJsonReader::DoubleJsonHandler _roughnessFactor;
  TextureInfoJsonHandler _metallicRoughnessTexture;
};
} // namespace CesiumGltfReader
