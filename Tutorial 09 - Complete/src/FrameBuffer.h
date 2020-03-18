#pragma once
#include <GLM/vec2.hpp>
#include <GLM/vec4.hpp>
#include <glad/glad.h>
#include "florp/graphics/IGraphicsResource.h"
#include "florp/graphics/TextureEnums.h"
#include "EnumToString.h"
#include <unordered_map>
#include "florp/graphics/Texture2D.h"

ENUM(RenderTargetAttachment, uint32_t,
     Color0       = GL_COLOR_ATTACHMENT0,
     Color1       = GL_COLOR_ATTACHMENT1,
     Color2       = GL_COLOR_ATTACHMENT2,
     Color3       = GL_COLOR_ATTACHMENT3,
     Color4       = GL_COLOR_ATTACHMENT4,
     Color5       = GL_COLOR_ATTACHMENT5,
     Color6       = GL_COLOR_ATTACHMENT6,
     Color7       = GL_COLOR_ATTACHMENT7,
     Depth        = GL_DEPTH_ATTACHMENT,
     DepthStencil = GL_DEPTH_STENCIL_ATTACHMENT,
     Stencil      = GL_STENCIL_ATTACHMENT
);

constexpr bool IsColorAttachment(RenderTargetAttachment attachment) {
	return attachment >= RenderTargetAttachment::Color0 && attachment <= RenderTargetAttachment::Color7;
}

ENUM(RenderTargetType, uint32_t,
	Color32      = GL_RGBA8,
	ColorRgb10   = GL_RGB10,
	ColorRgb8    = GL_RGB8,
	ColorRG8     = GL_RG8,
	ColorRed8    = GL_R8,
	ColorRgb16F  = GL_RGB16F, // NEW
	ColorRgba16F = GL_RGBA16F,
	DepthStencil = GL_DEPTH24_STENCIL8,
	Depth16      = GL_DEPTH_COMPONENT16,
	Depth24      = GL_DEPTH_COMPONENT24,
	Depth32      = GL_DEPTH_COMPONENT32,
	Stencil4     = GL_STENCIL_INDEX4,
	Stencil8     = GL_STENCIL_INDEX8,
	Stencil16    = GL_STENCIL_INDEX16,
	Default      = 0
);

ENUM_FLAGS(RenderTargetBinding, GLenum,
	None  = 0,
	Draw  = GL_DRAW_FRAMEBUFFER,
	Write = GL_DRAW_FRAMEBUFFER,
	Read  = GL_READ_FRAMEBUFFER,
	Both  = GL_FRAMEBUFFER
);

ENUM_FLAGS(BufferFlags, GLenum,
	None    = 0,
	Color   = GL_COLOR_BUFFER_BIT,
	Depth   = GL_DEPTH_BUFFER_BIT,
	Stencil = GL_STENCIL_BUFFER_BIT,
	All     = Color | Depth | Stencil
);

struct RenderBufferDesc {
	/*
	 * If this is set to true, we will generate an OpenGL texture instead of a render buffer
	 */
	bool ShaderReadable;
	/*
	 * The format internal format of the render buffer or texture
	 */
	RenderTargetType Format;
	/*
	 * Where the buffer will be attached to
	 */
	RenderTargetAttachment Attachment;
};

/*
 * Represents a buffer that can be rendered into. FrameBuffers can have multiple attachments, so we
 * can attach multiple images or RenderBuffers to it (for instance, depth, stencil, multiple color channels)
 *
 * Note: Sometimes this will be referred to as a RenderTarget (which is a bit more of a succinct description)
 */
class FrameBuffer : public florp::graphics::ITexture {
public:
	typedef std::shared_ptr<FrameBuffer> Sptr;

	/*
	 * Creates a new frame buffer with the given number of dimensions and optionally, a given number of samples (multi-sampling)
	 * @param width The width of the frame buffer, in texels (must be larger than 0)
	 * @param height The height of the frame buffer, in texels (must be larger than 0)
	 * @param numSamples The number of samples to use (must be greater than 0, and less than GL_MAX_SAMPLES). Default is 1
	 */
	FrameBuffer(uint32_t width, uint32_t height, uint8_t numSamples = 1);
	virtual ~FrameBuffer();

	// Gets the width of this frame buffer, in texels
	uint32_t GetWidth() const { return myWidth; }
	// Gets the height of this frame buffer, in texels
	uint32_t GetHeight() const { return myHeight; }
	// Gets the dimensions of this frame buffer, in texels
	glm::ivec2 GetSize() const { return { myWidth, myHeight }; }

	/*
	 * Gets the given attachment as a Texture, or nullptr if the attachment point cannot be retrieved as a texture
	 */
	florp::graphics::Texture2D::Sptr GetAttachment(RenderTargetAttachment attachment);

	/*
	 * Resizes this frame buffer to the new dimensions. Note that this will destroy any data currently stored within it,
	 * and invalidate any texture handles that have been retrieved from this frame buffer
	 * @param width The new width of the frame buffer (must be non-zero)
	 * @param height The new height of the frame buffer (must be non-zero)
	 */
	void Resize(uint32_t newWidth, uint32_t newHeight);

	/*
	 * Adds a new attachment to this frame buffer, with the given description. If a layer it already attached to the binding point
	 * given by the description, it will be destroyed
	 * @param desc The descriptor of the buffer that we want to add
	 */
	void AddAttachment(const RenderBufferDesc& desc);
	
	/*
	 * Validates this FrameBuffer, and returns whether it is in a valid state or not
	 * @returns True if the FrameBuffer is ready for drawing, false if otherwise
	 */
	bool Validate();

	virtual void Bind(uint32_t slot) override;
	virtual void Bind(uint32_t slot, RenderTargetAttachment attachment);
	
	/*
	 * Binds this frame buffer for usage as either a reading buffer, writing buffer, or both
	 * @param bindMode The slot to bind to (default is Draw/Write)
	 */
	void Bind(RenderTargetBinding bindMode = RenderTargetBinding::Draw) const;
	/*
	 * Unbinds this frame buffer from the slot that it is bound to. Note that you should always call UnBind before calling
	 * Bind on another frame buffer with the same parameters
	 */
	void UnBind() const;

	/*
		Blits (copies) the contents of the read framebuffer into the draw framebuffer
		@param srcBounds Region in the source framebuffer to copy from (left, top, right, bottom)
		@param dstBounds Region in the target framebuffer to copy to   (left, top, right, bottom)
		@param buffers   The buffers to copy
	*/
	static void Blit(
		const glm::ivec4& srcBounds, const glm::ivec4& dstBounds, 
		BufferFlags flags = BufferFlags::All, florp::graphics::MagFilter filterMode = florp::graphics::MagFilter::Linear);

	/*
	 * Creates a clone of this frame buffer, with the same size and attachments
	 */
	Sptr Clone() const;
	
	/*
	 * Overrides SetDebug name, so that we can send the name into OpenGL
	 * @param value The new debug name for this object
	 */
	virtual void SetDebugName(const std::string& value) override;
	
protected:
	// The dimensions of this frame buffer
	uint32_t myWidth, myHeight;
	// The number of samples to use if we have multisampling enabled
	uint8_t  myNumSamples;
	// Whether or not this frame buffer is in a valid state
	bool     isValid;
	// The current attachment points that this FrameBuffer is bound to
	mutable RenderTargetBinding myBinding;

	// We will store a pointer to another FBO if this one is multisampled
	Sptr                        myUnsampledFrameBuffer;

	// Stores our attachment information for a given render buffer attachment point
	struct RenderBuffer {
		GLuint           RendererID;
		florp::graphics::IGraphicsResource::Sptr Resource;
		bool             IsRenderBuffer;
		RenderBufferDesc Description;

		RenderBuffer();
	};
	// Stores our buffers per attachment point
	std::unordered_map<RenderTargetAttachment, RenderBuffer> myLayers;
	std::vector<RenderTargetAttachment> myDrawBuffers;  // NEW
};

