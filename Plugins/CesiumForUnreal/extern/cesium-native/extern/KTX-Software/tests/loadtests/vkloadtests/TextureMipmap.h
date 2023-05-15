/* -*- tab-width: 4; -*- */
/* vi: set sw=2 ts=4 expandtab: */

/*
 * Copyright 2017-2020 Mark Callow.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _TEXTURE_MIPMAP_H_
#define _TEXTURE_MIPMAP_H_

/**
 * @internal
 * @file TextureMipmap.h
 * @~English
 *
 * @brief Declaration of test sample for loading and displaying all the levels of a 2D mipmapped texture.
 *
 * @author Mark Callow, www.edgewise-consulting.com.
 */

#include <vector>

#include "InstancedSampleBase.h"

#include <ktxvulkan.h>
#include <glm/gtc/matrix_transform.hpp>

class TextureMipmap : public InstancedSampleBase
{
  public:
    TextureMipmap(VulkanContext& vkctx,
                 uint32_t width, uint32_t height,
                 const char* const szArgs,
                 const std::string sBasePath);

    static VulkanLoadTestSample*
    create(VulkanContext& vkctx,
           uint32_t width, uint32_t height,
           const char* const szArgs, const std::string sBasePath);
};

#endif /* _TEXTURE_MIPMAP_H_ */
