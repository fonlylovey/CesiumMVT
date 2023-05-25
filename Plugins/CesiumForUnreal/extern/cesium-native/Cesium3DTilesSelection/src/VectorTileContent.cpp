#include <Cesium3DTilesSelection/VectorTileContent.h>

namespace Cesium3DTilesSelection {
VectorTileRenderContent::VectorTileRenderContent(
    CesiumGltf::VectorModel&& model)
    : _model{std::move(model)},
      _pRenderResources{nullptr},
      _vectorOverlayDetails{},
      _credits{},
      _lodTransitionFadePercentage{0.0f} {}

const CesiumGltf::VectorModel&
VectorTileRenderContent::getModel() const noexcept {
  return _model;
}

CesiumGltf::VectorModel& VectorTileRenderContent::getModel() noexcept {
  return _model;
}

void VectorTileRenderContent::setModel(const CesiumGltf::VectorModel& model) {
  _model = model;
}

void VectorTileRenderContent::setModel(CesiumGltf::VectorModel&& model) {
  _model = std::move(model);
}

const VectorOverlayDetails&
VectorTileRenderContent::getVectorOverlayDetails() const noexcept {
  return this->_vectorOverlayDetails;
}

VectorOverlayDetails& VectorTileRenderContent::getVectorOverlayDetails() noexcept {
  return this->_vectorOverlayDetails;
}

void VectorTileRenderContent::setVectorOverlayDetails(
    const VectorOverlayDetails& rasterOverlayDetails) {
  this->_vectorOverlayDetails = rasterOverlayDetails;
}

void VectorTileRenderContent::setVectorOverlayDetails(
    VectorOverlayDetails&& rasterOverlayDetails) {
  this->_vectorOverlayDetails = std::move(rasterOverlayDetails);
}

const std::vector<Credit>& VectorTileRenderContent::getCredits() const noexcept {
  return this->_credits;
}

std::vector<Credit>& VectorTileRenderContent::getCredits() noexcept {
  return this->_credits;
}

void VectorTileRenderContent::setCredits(std::vector<Credit>&& credits) {
  this->_credits = std::move(credits);
}

void VectorTileRenderContent::setCredits(const std::vector<Credit>& credits) {
  this->_credits = credits;
}

void* VectorTileRenderContent::getRenderResources() const noexcept {
  return this->_pRenderResources;
}

void VectorTileRenderContent::setRenderResources(void* pRenderResources) noexcept {
  this->_pRenderResources = pRenderResources;
}

float VectorTileRenderContent::getLodTransitionFadePercentage() const noexcept {
  return _lodTransitionFadePercentage;
}

void VectorTileRenderContent::setLodTransitionFadePercentage(
    float percentage) noexcept {
  this->_lodTransitionFadePercentage = percentage;
}

VectorTileContent::VectorTileContent() : _contentKind{TileUnknownContent{}} {}

VectorTileContent::VectorTileContent(TileEmptyContent content)
    : _contentKind{content} {}

VectorTileContent::VectorTileContent(TileExternalContent content)
    : _contentKind{content} {}

void VectorTileContent::setContentKind(TileUnknownContent content) {
  _contentKind = content;
}

void VectorTileContent::setContentKind(TileEmptyContent content) {
  _contentKind = content;
}

void VectorTileContent::setContentKind(TileExternalContent content) {
  _contentKind = content;
}

void VectorTileContent::setContentKind(
    std::unique_ptr<VectorTileRenderContent>&& content) {
  _contentKind = std::move(content);
}

bool VectorTileContent::isUnknownContent() const noexcept {
  return std::holds_alternative<TileUnknownContent>(this->_contentKind);
}

bool VectorTileContent::isEmptyContent() const noexcept {
  return std::holds_alternative<TileEmptyContent>(this->_contentKind);
}

bool VectorTileContent::isExternalContent() const noexcept {
  return std::holds_alternative<TileExternalContent>(this->_contentKind);
}

bool VectorTileContent::isRenderContent() const noexcept {
  return std::holds_alternative<std::unique_ptr<VectorTileRenderContent>>(
      this->_contentKind);
}

const VectorTileRenderContent*
VectorTileContent::getRenderContent() const noexcept {
  const std::unique_ptr<VectorTileRenderContent>* pRenderContent =
      std::get_if<std::unique_ptr<VectorTileRenderContent>>(&this->_contentKind);
  if (pRenderContent) {
    return pRenderContent->get();
  }

  return nullptr;
}

VectorTileRenderContent* VectorTileContent::getRenderContent() noexcept {
  std::unique_ptr<VectorTileRenderContent>* pRenderContent =
      std::get_if<std::unique_ptr<VectorTileRenderContent>>(&this->_contentKind);
  if (pRenderContent) {
    return pRenderContent->get();
  }

  return nullptr;
}
} // namespace Cesium3DTilesSelection
