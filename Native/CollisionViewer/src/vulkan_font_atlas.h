#pragma once
// The atlas is immutable and embedded. Each swapchain uploads it once on its
// own graphics queue; subsequent frames submit one quad per glyph.
static uint32_t fontMemoryType(const Device& device,uint32_t bits,VkMemoryPropertyFlags flags) {
    for(uint32_t i=0;i<device.memory.memoryTypeCount;++i)
        if((bits&(1u<<i))&&(device.memory.memoryTypes[i].propertyFlags&flags)==flags) return i;
    throw std::runtime_error("No compatible font atlas memory type");
}
static void createFontAtlas(Swapchain& swap) {
    auto& d=*swap.device;
    VkImageCreateInfo image{VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO}; image.imageType=VK_IMAGE_TYPE_2D; image.format=VK_FORMAT_R8_UNORM;
    image.extent={kEternalUi2AtlasWidth,kEternalUi2AtlasHeight,1}; image.mipLevels=image.arrayLayers=1;
    image.samples=VK_SAMPLE_COUNT_1_BIT; image.tiling=VK_IMAGE_TILING_OPTIMAL;
    image.usage=VK_IMAGE_USAGE_TRANSFER_DST_BIT|VK_IMAGE_USAGE_SAMPLED_BIT;
    require(d.CreateImage(d.handle,&image,nullptr,&swap.fontImage),"Create font atlas image failed");
    VkMemoryRequirements requirements{}; d.GetImageMemoryRequirements(d.handle,swap.fontImage,&requirements);
    VkMemoryAllocateInfo memory{VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO}; memory.allocationSize=requirements.size;
    memory.memoryTypeIndex=fontMemoryType(d,requirements.memoryTypeBits,VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
    require(d.AllocateMemory(d.handle,&memory,nullptr,&swap.fontMemory),"Allocate font atlas memory failed");
    require(d.BindImageMemory(d.handle,swap.fontImage,swap.fontMemory,0),"Bind font atlas memory failed");
    VkImageViewCreateInfo view{VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO}; view.image=swap.fontImage; view.viewType=VK_IMAGE_VIEW_TYPE_2D; view.format=image.format;
    view.subresourceRange={VK_IMAGE_ASPECT_COLOR_BIT,0,1,0,1};
    require(d.CreateImageView(d.handle,&view,nullptr,&swap.fontView),"Create font atlas view failed");
    VkSamplerCreateInfo sampler{VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO}; sampler.minFilter=sampler.magFilter=VK_FILTER_LINEAR;
    sampler.mipmapMode=VK_SAMPLER_MIPMAP_MODE_NEAREST; sampler.addressModeU=sampler.addressModeV=sampler.addressModeW=VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    require(d.CreateSampler(d.handle,&sampler,nullptr,&swap.fontSampler),"Create font atlas sampler failed");
    const auto& pixels=eternalUi2AtlasPixels();
    VkBufferCreateInfo buffer{VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO}; buffer.size=pixels.size(); buffer.usage=VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
    require(d.CreateBuffer(d.handle,&buffer,nullptr,&swap.fontStaging),"Create font atlas staging failed");
    d.GetBufferMemoryRequirements(d.handle,swap.fontStaging,&requirements);
    memory.allocationSize=requirements.size;
    memory.memoryTypeIndex=fontMemoryType(d,requirements.memoryTypeBits,VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT|VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
    require(d.AllocateMemory(d.handle,&memory,nullptr,&swap.fontStagingMemory),"Allocate font atlas staging failed");
    require(d.BindBufferMemory(d.handle,swap.fontStaging,swap.fontStagingMemory,0),"Bind font atlas staging failed");
    void* mapped=nullptr;
    require(d.MapMemory(d.handle,swap.fontStagingMemory,0,pixels.size(),0,&mapped),"Map font atlas staging failed");
    memcpy(mapped,pixels.data(),pixels.size()); d.UnmapMemory(d.handle,swap.fontStagingMemory);
}
static void destroyFontAtlas(Swapchain& swap) {
    auto& d=*swap.device;
    if(swap.fontSampler) d.DestroySampler(d.handle,swap.fontSampler,nullptr);
    if(swap.fontView) d.DestroyImageView(d.handle,swap.fontView,nullptr);
    if(swap.fontImage) d.DestroyImage(d.handle,swap.fontImage,nullptr);
    if(swap.fontMemory) d.FreeMemory(d.handle,swap.fontMemory,nullptr);
    if(swap.fontStaging) d.DestroyBuffer(d.handle,swap.fontStaging,nullptr);
    if(swap.fontStagingMemory) d.FreeMemory(d.handle,swap.fontStagingMemory,nullptr);
    swap.fontSampler={}; swap.fontView={}; swap.fontImage={}; swap.fontMemory={};
    swap.fontStaging={}; swap.fontStagingMemory={}; swap.fontUploaded=false;
}
static void recordFontAtlasUpload(Swapchain& swap,VkCommandBuffer commands) {
    if(swap.fontUploaded) return;
    auto& d=*swap.device;
    VkImageMemoryBarrier image{VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER};
    image.oldLayout=VK_IMAGE_LAYOUT_UNDEFINED; image.newLayout=VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    image.dstAccessMask=VK_ACCESS_TRANSFER_WRITE_BIT; image.srcQueueFamilyIndex=image.dstQueueFamilyIndex=VK_QUEUE_FAMILY_IGNORED;
    image.image=swap.fontImage; image.subresourceRange={VK_IMAGE_ASPECT_COLOR_BIT,0,1,0,1};
    d.CmdPipelineBarrier(commands,VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,VK_PIPELINE_STAGE_TRANSFER_BIT,0,0,nullptr,0,nullptr,1,&image);
    VkBufferImageCopy copy{}; copy.imageSubresource={VK_IMAGE_ASPECT_COLOR_BIT,0,0,1}; copy.imageExtent={kEternalUi2AtlasWidth,kEternalUi2AtlasHeight,1};
    d.CmdCopyBufferToImage(commands,swap.fontStaging,swap.fontImage,VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,1,&copy);
    image.oldLayout=VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL; image.newLayout=VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    image.srcAccessMask=VK_ACCESS_TRANSFER_WRITE_BIT; image.dstAccessMask=VK_ACCESS_SHADER_READ_BIT;
    d.CmdPipelineBarrier(commands,VK_PIPELINE_STAGE_TRANSFER_BIT,VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,0,0,nullptr,0,nullptr,1,&image);
}
