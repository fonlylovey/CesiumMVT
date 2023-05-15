// This file was generated by generate-classes.
// DO NOT EDIT THIS FILE!
#pragma once

#include "TextureInfoJsonHandler.h"

#include <CesiumGltf/MaterialOcclusionTextureInfo.h>
#include <CesiumJsonReader/DoubleJsonHandler.h>

namespace CesiumJsonReader {
class ExtensionReaderContext;
}

namespace CesiumGltfReader {
class MaterialOcclusionTextureInfoJsonHandler : public TextureInfoJsonHandler {
public:
  using ValueType = CesiumGltf::MaterialOcclusionTextureInfo;

  MaterialOcclusionTextureInfoJsonHandler(
      const CesiumJsonReader::ExtensionReaderContext& context) noexcept;
  void reset(
      IJsonHandler* pParentHandler,
      CesiumGltf::MaterialOcclusionTextureInfo* pObject);

  virtual IJsonHandler* readObjectKey(const std::string_view& str) override;

protected:
  IJsonHandler* readObjectKeyMaterialOcclusionTextureInfo(
      const std::string& objectType,
      const std::string_view& str,
      CesiumGltf::MaterialOcclusionTextureInfo& o);

private:
  CesiumGltf::MaterialOcclusionTextureInfo* _pObject = nullptr;
  CesiumJsonReader::DoubleJsonHandler _strength;
};
} // namespace CesiumGltfReader
