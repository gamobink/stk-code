//  SuperTuxKart - a fun racing game with go-kart
//  Copyright (C) 2013-2015 Lauri Kasanen
//
//  This program is free software; you can redistribute it and/or
//  modify it under the terms of the GNU General Public License
//  as published by the Free Software Foundation; either version 3
//  of the License, or (at your option) any later version.
//
//  This program is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU General Public License for more details.
//
//  You should have received a copy of the GNU General Public License
//  along with this program; if not, write to the Free Software
//  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

#ifndef SERVER_ONLY

#include "graphics/rtts.hpp"

#include "config/user_config.hpp"
#include "graphics/central_settings.hpp"
#include "graphics/frame_buffer_layer.hpp"
#include "utils/log.hpp"

#include <dimension2d.h>

using namespace irr;
static GLuint generateRTT3D(GLenum target, unsigned int w, unsigned int h, 
                            unsigned int d, GLint internalFormat, GLint format,
                            GLint type, unsigned mipmaplevel = 1)
{
    GLuint result;
    glGenTextures(1, &result);
    glBindTexture(target, result);
    if (CVS->isARBTextureStorageUsable())
        glTexStorage3D(target, mipmaplevel, internalFormat, w, h, d);
    else
        glTexImage3D(target, 0, internalFormat, w, h, d, 0, format, type, 0);
    return result;
}

static GLuint generateRTT(const core::dimension2du &res, GLint internalFormat, GLint format, GLint type, unsigned mipmaplevel = 1)
{
    GLuint result;
    glGenTextures(1, &result);
    glBindTexture(GL_TEXTURE_2D, result);
    if (CVS->isARBTextureStorageUsable())
        glTexStorage2D(GL_TEXTURE_2D, mipmaplevel, internalFormat, res.Width, res.Height);
    else
        glTexImage2D(GL_TEXTURE_2D, 0, internalFormat, res.Width, res.Height, 0, format, type, 0);
    return result;
}

RTT::RTT(unsigned int width, unsigned int height, float rtt_scale)
{
    m_width = (unsigned int)(width * rtt_scale);
    m_height = (unsigned int)(height  * rtt_scale);
    m_shadow_FBO = NULL;
    using namespace video;
    using namespace core;

    dimension2du res(m_width, m_height);
    
    const dimension2du half = res/2;
    const dimension2du quarter = res/4;
    const dimension2du eighth = res/8;

    const u16 shadowside = u16(1024 * rtt_scale);
    const dimension2du shadowsize0(shadowside, shadowside);
    const dimension2du shadowsize1(shadowside / 2, shadowside / 2);
    const dimension2du shadowsize2(shadowside / 4, shadowside / 4);
    const dimension2du shadowsize3(shadowside / 8, shadowside / 8);

    unsigned linear_depth_mip_levels = int(ceilf(log2f( float(max_(res.Width, res.Height)) )));

    DepthStencilTexture = generateRTT(res, GL_DEPTH24_STENCIL8, GL_DEPTH_STENCIL, GL_UNSIGNED_INT_24_8);

    // All RTTs are currently RGBA16F mostly with stencil. The four tmp RTTs are the same size
    // as the screen, for use in post-processing.
    
    GLint rgba_internal_format = GL_RGBA16F;
    GLint rgba_format = GL_BGRA;
    GLint red_internal_format = GL_R16F;
    GLint red32_internal_format = GL_R32F;
    GLint red_format = GL_RED;
    GLint rgb_format = GL_BGR;
    GLint diffuse_specular_internal_format = GL_R11F_G11F_B10F;
    GLint type = GL_FLOAT;
    GLint rgb_internal_format = GL_RGBA8;
    
#if defined(USE_GLES2)
    if (!CVS->isEXTColorBufferFloatUsable())
    {
        rgba_internal_format = GL_RGBA8;
        rgba_format = GL_RGBA;
        red_internal_format = GL_R8;
        red32_internal_format = GL_R8;
        red_format = GL_RED;
        rgb_format = GL_RGB;
        diffuse_specular_internal_format = GL_RGBA8;
        type = GL_UNSIGNED_BYTE;
    }
#endif

    RenderTargetTextures[RTT_TMP1] = generateRTT(res, rgba_internal_format, rgba_format, type);
    RenderTargetTextures[RTT_TMP2] = generateRTT(res, rgba_internal_format, rgba_format, type);
    RenderTargetTextures[RTT_TMP3] = generateRTT(res, rgba_internal_format, rgba_format, type);
    RenderTargetTextures[RTT_TMP4] = generateRTT(res, red_internal_format, red_format, type);
    RenderTargetTextures[RTT_LINEAR_DEPTH] = generateRTT(res, red32_internal_format, red_format, type, linear_depth_mip_levels);
    RenderTargetTextures[RTT_NORMAL_AND_DEPTH] = generateRTT(res, GL_RGB10_A2, GL_RGBA, GL_UNSIGNED_INT_2_10_10_10_REV);
    RenderTargetTextures[RTT_COLOR] = generateRTT(res, rgba_internal_format, rgba_format, type);
    RenderTargetTextures[RTT_MLAA_COLORS] = generateRTT(res, rgb_internal_format, rgb_format, GL_UNSIGNED_BYTE);
    RenderTargetTextures[RTT_MLAA_TMP] = generateRTT(res, rgb_internal_format, rgb_format, GL_UNSIGNED_BYTE);
    RenderTargetTextures[RTT_MLAA_BLEND] = generateRTT(res, rgb_internal_format, rgb_format, GL_UNSIGNED_BYTE);
    RenderTargetTextures[RTT_SSAO] = generateRTT(res, red_internal_format, red_format, type);
    RenderTargetTextures[RTT_DISPLACE] = generateRTT(res, rgba_internal_format, rgba_format, type);
    RenderTargetTextures[RTT_DIFFUSE] = generateRTT(res, diffuse_specular_internal_format, rgb_format, type);
    RenderTargetTextures[RTT_SPECULAR] = generateRTT(res, diffuse_specular_internal_format, rgb_format, type);

    RenderTargetTextures[RTT_HALF1] = generateRTT(half, rgba_internal_format, rgba_format, type);
    RenderTargetTextures[RTT_QUARTER1] = generateRTT(quarter, rgba_internal_format, rgba_format, type);
    RenderTargetTextures[RTT_EIGHTH1] = generateRTT(eighth, rgba_internal_format, rgba_format, type);
    RenderTargetTextures[RTT_HALF1_R] = generateRTT(half, red_internal_format, red_format, type);

    RenderTargetTextures[RTT_HALF2] = generateRTT(half, rgba_internal_format, rgba_format, type);
    RenderTargetTextures[RTT_QUARTER2] = generateRTT(quarter, rgba_internal_format, rgba_format, type);
    RenderTargetTextures[RTT_EIGHTH2] = generateRTT(eighth, rgba_internal_format, rgba_format, type);
    RenderTargetTextures[RTT_HALF2_R] = generateRTT(half, red_internal_format, red_format, type);

    RenderTargetTextures[RTT_BLOOM_1024] = generateRTT(shadowsize0, rgba_internal_format, rgb_format, type);

    RenderTargetTextures[RTT_BLOOM_512] = generateRTT(shadowsize1, rgba_internal_format, rgb_format, type);
    RenderTargetTextures[RTT_TMP_512] = generateRTT(shadowsize1, rgba_internal_format, rgb_format, type);
    RenderTargetTextures[RTT_LENS_512] = generateRTT(shadowsize1, rgba_internal_format, rgb_format, type);
	
    RenderTargetTextures[RTT_BLOOM_256] = generateRTT(shadowsize2, rgba_internal_format, rgb_format, type);
    RenderTargetTextures[RTT_TMP_256] = generateRTT(shadowsize2, rgba_internal_format, rgb_format, type);
    RenderTargetTextures[RTT_LENS_256] = generateRTT(shadowsize2, rgba_internal_format, rgb_format, type);
	
    RenderTargetTextures[RTT_BLOOM_128] = generateRTT(shadowsize3, rgba_internal_format, rgb_format, type);
    RenderTargetTextures[RTT_TMP_128] = generateRTT(shadowsize3, rgba_internal_format, rgb_format, type);
    RenderTargetTextures[RTT_LENS_128] = generateRTT(shadowsize3, rgba_internal_format, rgb_format, type);

    RenderTargetTextures[RTT_SP_GLOSS] = generateRTT(res, GL_RG8, GL_RG, GL_UNSIGNED_BYTE);
    RenderTargetTextures[RTT_SP_DIFF_COLOR] = generateRTT(res, GL_RGBA8, rgb_format, GL_UNSIGNED_BYTE);

    std::vector<GLuint> somevector;
    somevector.push_back(RenderTargetTextures[RTT_SSAO]);
    FrameBuffers.push_back(new FrameBuffer(somevector, res.Width, res.Height));
    somevector.clear();
    somevector.push_back(RenderTargetTextures[RTT_NORMAL_AND_DEPTH]);
    FrameBuffers.push_back(new FrameBuffer(somevector, DepthStencilTexture, res.Width, res.Height));
    somevector.clear();
    somevector.push_back(RenderTargetTextures[RTT_DIFFUSE]);
    somevector.push_back(RenderTargetTextures[RTT_SPECULAR]);
    FrameBuffers.push_back(new FrameBuffer(somevector, DepthStencilTexture, res.Width, res.Height));
    somevector.clear();
    somevector.push_back(RenderTargetTextures[RTT_COLOR]);
    FrameBuffers.push_back(new FrameBuffer(somevector, DepthStencilTexture, res.Width, res.Height));
    somevector.clear();
    somevector.push_back(RenderTargetTextures[RTT_DIFFUSE]);
    FrameBuffers.push_back(new FrameBuffer(somevector, res.Width, res.Height));
    somevector.clear();
    somevector.push_back(RenderTargetTextures[RTT_SPECULAR]);
    FrameBuffers.push_back(new FrameBuffer(somevector, res.Width, res.Height));
    somevector.clear();
    somevector.push_back(RenderTargetTextures[RTT_MLAA_COLORS]);
    FrameBuffers.push_back(new FrameBuffer(somevector, res.Width, res.Height));
    somevector.clear();
    somevector.push_back(RenderTargetTextures[RTT_MLAA_BLEND]);
    FrameBuffers.push_back(new FrameBuffer(somevector, res.Width, res.Height));
    somevector.clear();
    somevector.push_back(RenderTargetTextures[RTT_MLAA_TMP]);
    FrameBuffers.push_back(new FrameBuffer(somevector, res.Width, res.Height));
    somevector.clear();
    somevector.push_back(RenderTargetTextures[RTT_TMP1]);
    FrameBuffers.push_back(new FrameBuffer(somevector, DepthStencilTexture, res.Width, res.Height));
    somevector.clear();
    somevector.push_back(RenderTargetTextures[RTT_TMP2]);
    FrameBuffers.push_back(new FrameBuffer(somevector, DepthStencilTexture, res.Width, res.Height));
    somevector.clear();
    somevector.push_back(RenderTargetTextures[RTT_TMP4]);
    FrameBuffers.push_back(new FrameBuffer(somevector, res.Width, res.Height));
    somevector.clear();
    somevector.push_back(RenderTargetTextures[RTT_LINEAR_DEPTH]);
    FrameBuffers.push_back(new FrameBuffer(somevector, res.Width, res.Height));
    somevector.clear();
    
    somevector.push_back(RenderTargetTextures[RTT_HALF1]);
    FrameBuffers.push_back(new FrameBuffer(somevector, half.Width, half.Height));
    somevector.clear();
    somevector.push_back(RenderTargetTextures[RTT_HALF1_R]);
    FrameBuffers.push_back(new FrameBuffer(somevector, half.Width, half.Height));
    somevector.clear();
    somevector.push_back(RenderTargetTextures[RTT_HALF2]);
    FrameBuffers.push_back(new FrameBuffer(somevector, half.Width, half.Height));
    somevector.clear();
    somevector.push_back(RenderTargetTextures[RTT_HALF2_R]);
    FrameBuffers.push_back(new FrameBuffer(somevector, half.Width, half.Height));
    somevector.clear();
    somevector.push_back(RenderTargetTextures[RTT_QUARTER1]);
    FrameBuffers.push_back(new FrameBuffer(somevector, quarter.Width, quarter.Height));
    somevector.clear();
    somevector.push_back(RenderTargetTextures[RTT_QUARTER2]);
    FrameBuffers.push_back(new FrameBuffer(somevector, quarter.Width, quarter.Height));
    somevector.clear();
    somevector.push_back(RenderTargetTextures[RTT_EIGHTH1]);
    FrameBuffers.push_back(new FrameBuffer(somevector, eighth.Width, eighth.Height));
    somevector.clear();
    somevector.push_back(RenderTargetTextures[RTT_EIGHTH2]);
    FrameBuffers.push_back(new FrameBuffer(somevector, eighth.Width, eighth.Height));
    somevector.clear();
    somevector.push_back(RenderTargetTextures[RTT_DISPLACE]);
    FrameBuffers.push_back(new FrameBuffer(somevector, DepthStencilTexture, res.Width, res.Height));
    somevector.clear();
    
    somevector.push_back(RenderTargetTextures[RTT_BLOOM_1024]);
    FrameBuffers.push_back(new FrameBuffer(somevector, shadowsize0.Width, shadowsize0.Height));
    somevector.clear();

    somevector.push_back(RenderTargetTextures[RTT_BLOOM_512]);
    FrameBuffers.push_back(new FrameBuffer(somevector, shadowsize1.Width, shadowsize1.Height));
    somevector.clear();
    somevector.push_back(RenderTargetTextures[RTT_TMP_512]);
    FrameBuffers.push_back(new FrameBuffer(somevector, shadowsize1.Width, shadowsize1.Height));
	somevector.clear();
    somevector.push_back(RenderTargetTextures[RTT_LENS_512]);
    FrameBuffers.push_back(new FrameBuffer(somevector, shadowsize1.Width, shadowsize1.Height));
    somevector.clear();
    
    somevector.push_back(RenderTargetTextures[RTT_BLOOM_256]);
    FrameBuffers.push_back(new FrameBuffer(somevector, shadowsize2.Width, shadowsize2.Height));
    somevector.clear();
    somevector.push_back(RenderTargetTextures[RTT_TMP_256]);
    FrameBuffers.push_back(new FrameBuffer(somevector, shadowsize2.Width, shadowsize2.Height));
	somevector.clear();
    somevector.push_back(RenderTargetTextures[RTT_LENS_256]);
    FrameBuffers.push_back(new FrameBuffer(somevector, shadowsize2.Width, shadowsize2.Height));
    somevector.clear();
    
    somevector.push_back(RenderTargetTextures[RTT_BLOOM_128]);
    FrameBuffers.push_back(new FrameBuffer(somevector, shadowsize3.Width, shadowsize3.Height));
    somevector.clear();
    somevector.push_back(RenderTargetTextures[RTT_TMP_128]);
    FrameBuffers.push_back(new FrameBuffer(somevector, shadowsize3.Width, shadowsize3.Height));
	somevector.clear();
    somevector.push_back(RenderTargetTextures[RTT_LENS_128]);
    FrameBuffers.push_back(new FrameBuffer(somevector, shadowsize3.Width, shadowsize3.Height));

	somevector.clear();
    somevector.push_back(RenderTargetTextures[RTT_SP_DIFF_COLOR]);
    somevector.push_back(RenderTargetTextures[RTT_NORMAL_AND_DEPTH]);
    somevector.push_back(RenderTargetTextures[RTT_SP_GLOSS]);
    FrameBuffers.push_back(new FrameBuffer(somevector, DepthStencilTexture, res.Width, res.Height));

    if (CVS->isShadowEnabled())
    {
        shadowDepthTex = generateRTT3D(GL_TEXTURE_2D_ARRAY, UserConfigParams::m_shadows_resolution, UserConfigParams::m_shadows_resolution, 4, GL_DEPTH24_STENCIL8, GL_DEPTH_STENCIL, GL_UNSIGNED_INT_24_8, 1);
        somevector.clear();
        m_shadow_FBO = new FrameBufferLayer(somevector, shadowDepthTex, UserConfigParams::m_shadows_resolution, UserConfigParams::m_shadows_resolution, 4);
    }

    // Clear this FBO to 1s so that if no SSAO is computed we can still use it.
    getFBO(FBO_HALF1_R).bind();
    glClearColor(1., 1., 1., 1.);
    glClear(GL_COLOR_BUFFER_BIT);

    getFBO(FBO_COMBINED_DIFFUSE_SPECULAR).bind();
    float color = 0.5;
#if defined(USE_GLES2)
    if (!CVS->isDefferedEnabled())
        color = pow(color, 1. / 2.2);
#endif
    glClearColor(color, color, color, color);
    glClear(GL_COLOR_BUFFER_BIT);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

RTT::~RTT()
{
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glDeleteTextures(RTT_COUNT, RenderTargetTextures);
    glDeleteTextures(1, &DepthStencilTexture);
    if (CVS->isShadowEnabled())
    {
        delete m_shadow_FBO;
        glDeleteTextures(1, &shadowDepthTex);
    }
}

#endif   // !SERVER_ONLY
