// Copyright 2015 XLGAMES Inc.
//
// Distributed under the MIT License (See
// accompanying file "LICENSE" or the website
// http://www.opensource.org/licenses/mit-license.php)

#include "ObjectFactory.h"
#include "../../../Core/Prefix.h"

namespace RenderCore { namespace Metal_Vulkan
{
    const VkAllocationCallbacks* g_allocationCallbacks = nullptr;

    static std::shared_ptr<IDestructionQueue> CreateDestructionQueue(VulkanSharedPtr<VkDevice> device);

    VulkanUniquePtr<VkCommandPool> ObjectFactory::CreateCommandPool(
        unsigned queueFamilyIndex, VkCommandPoolCreateFlags flags) const
    {
        VkCommandPoolCreateInfo createInfo = {};
		createInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
		createInfo.pNext = nullptr;
		createInfo.queueFamilyIndex = queueFamilyIndex;
		createInfo.flags = flags;

        auto d = _destruction.get();
		VkCommandPool rawPool = nullptr;
		auto res = vkCreateCommandPool(_device.get(), &createInfo, g_allocationCallbacks, &rawPool);
		auto pool = VulkanUniquePtr<VkCommandPool>(
			rawPool,
			[d](VkCommandPool pool) { d->Destroy(pool); });
		if (res != VK_SUCCESS)
			Throw(VulkanAPIFailure(res, "Failure while creating command pool"));
        return std::move(pool);
    }

    VulkanUniquePtr<VkSemaphore> ObjectFactory::CreateSemaphore(
        VkSemaphoreCreateFlags flags) const
    {
        VkSemaphoreCreateInfo createInfo = {};
        createInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
        createInfo.pNext = nullptr;
        createInfo.flags = flags;

        auto d = _destruction.get();
        VkSemaphore rawPtr = nullptr;
        auto res = vkCreateSemaphore(
            _device.get(), &createInfo,
            g_allocationCallbacks, &rawPtr);
        VulkanUniquePtr<VkSemaphore> result(
            rawPtr,
            [d](VkSemaphore sem) { d->Destroy(sem); });
        if (res != VK_SUCCESS)
            Throw(VulkanAPIFailure(res, "Failure while creating Vulkan semaphore"));
        return std::move(result);
    }

    VulkanUniquePtr<VkDeviceMemory> ObjectFactory::AllocateMemory(
        VkDeviceSize allocationSize, unsigned memoryTypeIndex) const
    {
        VkMemoryAllocateInfo mem_alloc = {};
        mem_alloc.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        mem_alloc.pNext = nullptr;
        mem_alloc.allocationSize = allocationSize;
        mem_alloc.memoryTypeIndex = memoryTypeIndex;

        auto d = _destruction.get();
        VkDeviceMemory rawMem = nullptr;
        auto res = vkAllocateMemory(_device.get(), &mem_alloc, g_allocationCallbacks, &rawMem);
        auto mem = VulkanUniquePtr<VkDeviceMemory>(
            rawMem,
            [d](VkDeviceMemory mem) { d->Destroy(mem); });
        if (res != VK_SUCCESS)
            Throw(VulkanAPIFailure(res, "Failed while allocating device memory for image"));

        return std::move(mem);
    }

    VulkanUniquePtr<VkRenderPass> ObjectFactory::CreateRenderPass(
        const VkRenderPassCreateInfo& createInfo) const
    {
        auto d = _destruction.get();
        VkRenderPass rawPtr = nullptr;
        auto res = vkCreateRenderPass(_device.get(), &createInfo, g_allocationCallbacks, &rawPtr);
        auto renderPass = VulkanUniquePtr<VkRenderPass>(
            rawPtr,
            [d](VkRenderPass pass) { d->Destroy(pass); });
        if (res != VK_SUCCESS)
            Throw(VulkanAPIFailure(res, "Failure while creating render pass"));
        return std::move(renderPass);
    }

    VulkanUniquePtr<VkImage> ObjectFactory::CreateImage(
        const VkImageCreateInfo& createInfo) const
    {
        auto d = _destruction.get();
        VkImage rawImage = nullptr;
        auto res = vkCreateImage(_device.get(), &createInfo, g_allocationCallbacks, &rawImage);
        auto image = VulkanUniquePtr<VkImage>(
            rawImage,
            [d](VkImage image) { d->Destroy(image); });
        if (res != VK_SUCCESS)
            Throw(VulkanAPIFailure(res, "Failed while creating image"));
        return std::move(image);
    }

    VulkanUniquePtr<VkImageView> ObjectFactory::CreateImageView(
        const VkImageViewCreateInfo& createInfo) const
    {
        auto d = _destruction.get();
        VkImageView viewRaw = nullptr;
        auto result = vkCreateImageView(_device.get(), &createInfo, g_allocationCallbacks, &viewRaw);
        auto imageView = VulkanUniquePtr<VkImageView>(
            viewRaw,
            [d](VkImageView view) { d->Destroy(view); });
        if (result != VK_SUCCESS)
            Throw(VulkanAPIFailure(result, "Failed while creating depth stencil view of resource"));
        return std::move(imageView);
    }

    VulkanUniquePtr<VkFramebuffer> ObjectFactory::CreateFramebuffer(
        const VkFramebufferCreateInfo& createInfo) const
    {
        auto d = _destruction.get();
        VkFramebuffer rawFB = nullptr;
        auto res = vkCreateFramebuffer(_device.get(), &createInfo, g_allocationCallbacks, &rawFB);
        auto framebuffer = VulkanUniquePtr<VkFramebuffer>(
            rawFB,
            [d](VkFramebuffer fb) { d->Destroy(fb); });
        if (res != VK_SUCCESS)
            Throw(VulkanAPIFailure(res, "Failed while allocating frame buffer"));
        return std::move(framebuffer);
    }

    VulkanUniquePtr<VkShaderModule> ObjectFactory::CreateShaderModule(
        const void* byteCode, size_t size,
        VkShaderModuleCreateFlags flags) const
    {
        VkShaderModuleCreateInfo createInfo = {};
        createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
        createInfo.pNext = nullptr;
        createInfo.flags = flags;
        createInfo.codeSize = size;
        createInfo.pCode = (const uint32_t*)byteCode;

        auto d = _destruction.get();
        VkShaderModule rawShader = nullptr;
        auto res = vkCreateShaderModule(_device.get(), &createInfo, g_allocationCallbacks, &rawShader);
        auto shader = VulkanUniquePtr<VkShaderModule>(
            rawShader,
            [d](VkShaderModule shdr) { d->Destroy(shdr); });
        if (res != VK_SUCCESS)
            Throw(VulkanAPIFailure(res, "Failed while creating shader module"));
        return std::move(shader);
    }

    VulkanUniquePtr<VkDescriptorSetLayout> ObjectFactory::CreateDescriptorSetLayout(
        IteratorRange<const VkDescriptorSetLayoutBinding*> bindings) const
    {
        VkDescriptorSetLayoutCreateInfo createInfo = {};
        createInfo.sType =
            VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
        createInfo.pNext = nullptr;
        createInfo.bindingCount = (uint32)bindings.size();
        createInfo.pBindings = bindings.begin();

        auto d = _destruction.get();
        VkDescriptorSetLayout rawLayout = nullptr;
        auto res = vkCreateDescriptorSetLayout(_device.get(), &createInfo, g_allocationCallbacks, &rawLayout);
        auto shader = VulkanUniquePtr<VkDescriptorSetLayout>(
            rawLayout,
            [d](VkDescriptorSetLayout layout) { d->Destroy(layout); });
        if (res != VK_SUCCESS)
            Throw(VulkanAPIFailure(res, "Failed while creating descriptor set layout"));
        return std::move(shader);
    }

    VulkanUniquePtr<VkDescriptorPool> ObjectFactory::CreateDescriptorPool(
        const VkDescriptorPoolCreateInfo& createInfo) const
    {
        auto d = _destruction.get();
        VkDescriptorPool rawPool = nullptr;
        auto res = vkCreateDescriptorPool(_device.get(), &createInfo, g_allocationCallbacks, &rawPool);
        auto pool = VulkanUniquePtr<VkDescriptorPool>(
            rawPool,
            [d](VkDescriptorPool pool) { d->Destroy(pool); });
        if (res != VK_SUCCESS)
            Throw(VulkanAPIFailure(res, "Failed while creating descriptor pool"));
        return std::move(pool);
    }

    VulkanUniquePtr<VkPipeline> ObjectFactory::CreateGraphicsPipeline(
        VkPipelineCache pipelineCache,
        const VkGraphicsPipelineCreateInfo& createInfo) const
    {
        auto d = _destruction.get();
        VkPipeline rawPipeline = nullptr;
        auto res = vkCreateGraphicsPipelines(_device.get(), pipelineCache, 1, &createInfo, g_allocationCallbacks, &rawPipeline);
        auto pipeline = VulkanUniquePtr<VkPipeline>(
            rawPipeline,
            [d](VkPipeline pipeline) { d->Destroy(pipeline); });
        if (res != VK_SUCCESS)
            Throw(VulkanAPIFailure(res, "Failed while creating graphics pipeline"));
        return std::move(pipeline);
    }

    VulkanUniquePtr<VkPipelineCache> ObjectFactory::CreatePipelineCache(
        const void* initialData, size_t initialDataSize,
        VkPipelineCacheCreateFlags flags) const
    {
        VkPipelineCacheCreateInfo createInfo;
        createInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_CACHE_CREATE_INFO;
        createInfo.pNext = nullptr;
        createInfo.initialDataSize = initialDataSize;
        createInfo.pInitialData = initialData;
        createInfo.flags = flags;

        auto d = _destruction.get();
        VkPipelineCache rawCache = nullptr;
        auto res = vkCreatePipelineCache(_device.get(), &createInfo, g_allocationCallbacks, &rawCache);
        auto cache = VulkanUniquePtr<VkPipelineCache>(
            rawCache,
            [d](VkPipelineCache cache) { d->Destroy(cache); });
        if (res != VK_SUCCESS)
            Throw(VulkanAPIFailure(res, "Failed while creating descriptor set layout"));
        return std::move(cache);
    }

    VulkanUniquePtr<VkPipelineLayout> ObjectFactory::CreatePipelineLayout(
        IteratorRange<const VkDescriptorSetLayout*> setLayouts,
        IteratorRange<const VkPushConstantRange*> pushConstants,
        VkPipelineLayoutCreateFlags flags) const
    {
        VkPipelineLayoutCreateInfo pPipelineLayoutCreateInfo = {};
        pPipelineLayoutCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
        pPipelineLayoutCreateInfo.pNext = nullptr;
        pPipelineLayoutCreateInfo.flags = flags;
        pPipelineLayoutCreateInfo.setLayoutCount = (uint32_t)setLayouts.size();
        pPipelineLayoutCreateInfo.pSetLayouts = setLayouts.begin();
        pPipelineLayoutCreateInfo.pushConstantRangeCount = (uint32_t)pushConstants.size();
        pPipelineLayoutCreateInfo.pPushConstantRanges = pushConstants.begin();

        auto d = _destruction.get();
        VkPipelineLayout rawPipelineLayout = nullptr;
        auto res = vkCreatePipelineLayout(_device.get(), &pPipelineLayoutCreateInfo, g_allocationCallbacks, &rawPipelineLayout);
        auto pipelineLayout = VulkanUniquePtr<VkPipelineLayout>(
            rawPipelineLayout,
            [d](VkPipelineLayout layout) { d->Destroy(layout); });
        if (res != VK_SUCCESS)
            Throw(VulkanAPIFailure(res, "Failed while creating descriptor set layout"));
        return std::move(pipelineLayout);
    }

    VulkanUniquePtr<VkBuffer> ObjectFactory::CreateBuffer(const VkBufferCreateInfo& createInfo) const
    {
        auto d = _destruction.get();
        VkBuffer rawBuffer = nullptr;
        auto res = vkCreateBuffer(_device.get(), &createInfo, g_allocationCallbacks, &rawBuffer);
        auto buffer = VulkanUniquePtr<VkBuffer>(
            rawBuffer,
            [d](VkBuffer buffer) { d->Destroy(buffer); });
        if (res != VK_SUCCESS)
            Throw(VulkanAPIFailure(res, "Failed while creating buffer"));
        return std::move(buffer);
    }

    VulkanUniquePtr<VkFence> ObjectFactory::CreateFence(VkFenceCreateFlags flags) const
    {
        VkFenceCreateInfo createInfo = {};
        createInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
        createInfo.pNext = nullptr;
        createInfo.flags = flags;

        auto d = _destruction.get();
        VkFence rawFence = nullptr;
        auto res = vkCreateFence(_device.get(), &createInfo, g_allocationCallbacks, &rawFence);
        auto fence = VulkanUniquePtr<VkFence>(
            rawFence,
            [d](VkFence fence) { d->Destroy(fence); });
        if (res != VK_SUCCESS)
            Throw(VulkanAPIFailure(res, "Failed while creating fence"));
        return std::move(fence);
    }

    VulkanUniquePtr<VkSampler> ObjectFactory::CreateSampler(const VkSamplerCreateInfo& createInfo) const
    {
        auto d = _destruction.get();
        VkSampler rawSampler = nullptr;
        auto res = vkCreateSampler(_device.get(), &createInfo, g_allocationCallbacks, &rawSampler);
        auto sampler = VulkanUniquePtr<VkSampler>(
            rawSampler,
            [d](VkSampler sampler) { d->Destroy(sampler); });
        if (res != VK_SUCCESS)
            Throw(VulkanAPIFailure(res, "Failed while creating sampler"));
        return std::move(sampler);
    }

    unsigned ObjectFactory::FindMemoryType(VkFlags memoryTypeBits, VkMemoryPropertyFlags requirementsMask) const
    {
        // Search memtypes to find first index with those properties
        for (uint32_t i=0; i<dimof(_memProps.memoryTypes); i++) {
            if ((memoryTypeBits & 1) == 1) {
                // Type is available, does it match user properties?
                if ((_memProps.memoryTypes[i].propertyFlags & requirementsMask) == requirementsMask)
                    return i;
            }
            memoryTypeBits >>= 1;
        }
        return ~0x0u;
    }

	VkFormatProperties ObjectFactory::GetFormatProperties(VkFormat fmt) const
	{
		VkFormatProperties formatProps;
		vkGetPhysicalDeviceFormatProperties(_physDev, fmt, &formatProps);
		return formatProps;
	}

    void ObjectFactory::FlushDestructionQueue() const
    {
        _destruction->Flush();
    }

    ObjectFactory::ObjectFactory(VkPhysicalDevice physDev, VulkanSharedPtr<VkDevice> device)
    : _physDev(physDev), _device(device)
    {
        _memProps = {};
        vkGetPhysicalDeviceMemoryProperties(physDev, &_memProps);
        _destruction = CreateDestructionQueue(_device);
    }

    ObjectFactory::ObjectFactory(IDevice*) {}
    ObjectFactory::ObjectFactory(Underlying::Resource&) {}

	ObjectFactory::ObjectFactory() {}
	ObjectFactory::~ObjectFactory() 
    {
        if (_destruction)
            _destruction->Flush();
    }


    static const ObjectFactory* s_defaultObjectFactory = nullptr;

    void SetDefaultObjectFactory(const ObjectFactory* factory)
    {
        s_defaultObjectFactory = factory;
    }

    const ObjectFactory& GetDefaultObjectFactory()
    {
        return *s_defaultObjectFactory;
    }

///////////////////////////////////////////////////////////////////////////////////////////////////

    IDestructionQueue::~IDestructionQueue() {}

    class DeferredDestruction : public IDestructionQueue
    {
    public:
        void    Destroy(VkCommandPool);
        void    Destroy(VkSemaphore);
        void    Destroy(VkDeviceMemory);
        void    Destroy(VkRenderPass);
        void    Destroy(VkImage);
        void    Destroy(VkImageView);
        void    Destroy(VkFramebuffer);
        void    Destroy(VkShaderModule);
        void    Destroy(VkDescriptorSetLayout);
        void    Destroy(VkDescriptorPool);
        void    Destroy(VkPipeline);
        void    Destroy(VkPipelineCache);
        void    Destroy(VkPipelineLayout);
        void    Destroy(VkBuffer);
        void    Destroy(VkFence);
        void    Destroy(VkSampler);

        void    Flush();

        DeferredDestruction(VulkanSharedPtr<VkDevice> device);
        ~DeferredDestruction();
    private:
        VulkanSharedPtr<VkDevice> _device;

        template<typename Type>
            using Queue = std::vector<Type>;

        std::tuple<
              Queue<VkCommandPool>              // 0
            , Queue<VkSemaphore>                // 1
            , Queue<VkDeviceMemory>             // 2
            , Queue<VkRenderPass>               // 3
            , Queue<VkImage>                    // 4
            , Queue<VkImageView>                // 5
            , Queue<VkFramebuffer>              // 6
            , Queue<VkShaderModule>             // 7
            , Queue<VkDescriptorSetLayout>      // 8
            , Queue<VkDescriptorPool>           // 9
            , Queue<VkPipeline>                 // 10
            , Queue<VkPipelineCache>            // 11
            , Queue<VkPipelineLayout>           // 12
            , Queue<VkBuffer>                   // 13
            , Queue<VkFence>                    // 14
            , Queue<VkSampler>                  // 15
        > _queues;

        template<int Index, typename Type>
            void DoDestroy(Type obj);

        template<int Index>
            void FlushQueue();
    };

    template<int Index, typename Type>
        inline void DeferredDestruction::DoDestroy(Type obj)
    {
        //  Note -- we need std::get<Type> access.. But that requires C++14, and
        //          isn't supported on VS2013
        auto& q = std::get<Index>(_queues);
        q.push_back(obj);
    }

    template<typename Type> void DestroyObjectImmediate(VkDevice device, Type obj);
    template<> inline void DestroyObjectImmediate(VkDevice device, VkCommandPool obj)           { vkDestroyCommandPool(device, obj, g_allocationCallbacks ); }
    template<> inline void DestroyObjectImmediate(VkDevice device, VkSemaphore obj)             { vkDestroySemaphore(device, obj, g_allocationCallbacks ); }
    template<> inline void DestroyObjectImmediate(VkDevice device, VkDeviceMemory obj)          { vkFreeMemory(device, obj, g_allocationCallbacks ); }
    template<> inline void DestroyObjectImmediate(VkDevice device, VkRenderPass obj)            { vkDestroyRenderPass(device, obj, g_allocationCallbacks ); }
    template<> inline void DestroyObjectImmediate(VkDevice device, VkImage obj)                 { vkDestroyImage(device, obj, g_allocationCallbacks ); }
    template<> inline void DestroyObjectImmediate(VkDevice device, VkImageView obj)             { vkDestroyImageView(device, obj, g_allocationCallbacks ); }
    template<> inline void DestroyObjectImmediate(VkDevice device, VkFramebuffer obj)           { vkDestroyFramebuffer(device, obj, g_allocationCallbacks ); }
    template<> inline void DestroyObjectImmediate(VkDevice device, VkShaderModule obj)          { vkDestroyShaderModule(device, obj, g_allocationCallbacks ); }
    template<> inline void DestroyObjectImmediate(VkDevice device, VkDescriptorSetLayout obj)   { vkDestroyDescriptorSetLayout(device, obj, g_allocationCallbacks ); }
    template<> inline void DestroyObjectImmediate(VkDevice device, VkDescriptorPool obj)        { vkDestroyDescriptorPool(device, obj, g_allocationCallbacks ); }
    template<> inline void DestroyObjectImmediate(VkDevice device, VkPipeline obj)              { vkDestroyPipeline(device, obj, g_allocationCallbacks ); }
    template<> inline void DestroyObjectImmediate(VkDevice device, VkPipelineCache obj)         { vkDestroyPipelineCache(device, obj, g_allocationCallbacks ); }
    template<> inline void DestroyObjectImmediate(VkDevice device, VkPipelineLayout obj)        { vkDestroyPipelineLayout(device, obj, g_allocationCallbacks ); }
    template<> inline void DestroyObjectImmediate(VkDevice device, VkBuffer obj)                { vkDestroyBuffer(device, obj, g_allocationCallbacks ); }
    template<> inline void DestroyObjectImmediate(VkDevice device, VkFence obj)                 { vkDestroyFence(device, obj, g_allocationCallbacks ); }
    template<> inline void DestroyObjectImmediate(VkDevice device, VkSampler obj)               { vkDestroySampler(device, obj, g_allocationCallbacks ); }

    template<int Index>
        inline void DeferredDestruction::FlushQueue()
    {
        auto& q = std::get<Index>(_queues);
        for (auto i:q)
            DestroyObjectImmediate(_device.get(), i);
        q.clear();
    }

    void    DeferredDestruction::Destroy(VkCommandPool obj) { DoDestroy<0>(obj); }
    void    DeferredDestruction::Destroy(VkSemaphore obj) { DoDestroy<1>(obj); }
    void    DeferredDestruction::Destroy(VkDeviceMemory obj) { DoDestroy<2>(obj); }
    void    DeferredDestruction::Destroy(VkRenderPass obj) { DoDestroy<3>(obj); }
    void    DeferredDestruction::Destroy(VkImage obj) { DoDestroy<4>(obj); }
    void    DeferredDestruction::Destroy(VkImageView obj) { DoDestroy<5>(obj); }
    void    DeferredDestruction::Destroy(VkFramebuffer obj) { DoDestroy<6>(obj); }
    void    DeferredDestruction::Destroy(VkShaderModule obj) { DoDestroy<7>(obj); }
    void    DeferredDestruction::Destroy(VkDescriptorSetLayout obj) { DoDestroy<8>(obj); }
    void    DeferredDestruction::Destroy(VkDescriptorPool obj) { DoDestroy<9>(obj); }
    void    DeferredDestruction::Destroy(VkPipeline obj) { DoDestroy<10>(obj); }
    void    DeferredDestruction::Destroy(VkPipelineCache obj) { DoDestroy<11>(obj); }
    void    DeferredDestruction::Destroy(VkPipelineLayout obj) { DoDestroy<12>(obj); }
    void    DeferredDestruction::Destroy(VkBuffer obj) { DoDestroy<13>(obj); }
    void    DeferredDestruction::Destroy(VkFence obj) { DoDestroy<14>(obj); }
    void    DeferredDestruction::Destroy(VkSampler obj) { DoDestroy<15>(obj); }

    void    DeferredDestruction::Flush()
    {
        FlushQueue<0>();
        FlushQueue<1>();
        FlushQueue<2>();
        FlushQueue<3>();
        FlushQueue<4>();
        FlushQueue<5>();
        FlushQueue<6>();
        FlushQueue<7>();
        FlushQueue<8>();
        FlushQueue<9>();
        FlushQueue<10>();
        FlushQueue<11>();
        FlushQueue<12>();
        FlushQueue<13>();
        FlushQueue<14>();
        FlushQueue<15>();
    }

    DeferredDestruction::DeferredDestruction(VulkanSharedPtr<VkDevice> device)
    : _device(device)
    {
    }

    DeferredDestruction::~DeferredDestruction() 
    {
        Flush();
    }

    static std::shared_ptr<IDestructionQueue> CreateDestructionQueue(VulkanSharedPtr<VkDevice> device)
    {
        return std::make_shared<DeferredDestruction>(std::move(device));
    }

///////////////////////////////////////////////////////////////////////////////////////////////////

    static const char* AsString(VkResult res)
    {
        // String names for standard vulkan error codes
        switch (res)
        {
                // success codes
            case VK_SUCCESS: return "Success";
            case VK_NOT_READY: return "Not Ready";
            case VK_TIMEOUT: return "Timeout";
            case VK_EVENT_SET: return "Event Set";
            case VK_EVENT_RESET: return "Event Reset";
            case VK_INCOMPLETE: return "Incomplete";

                // error codes
            case VK_ERROR_OUT_OF_HOST_MEMORY: return "Out of host memory";
            case VK_ERROR_OUT_OF_DEVICE_MEMORY: return "Out of device memory";
            case VK_ERROR_INITIALIZATION_FAILED: return "Initialization failed";
            case VK_ERROR_DEVICE_LOST: return "Device lost";
            case VK_ERROR_MEMORY_MAP_FAILED: return "Memory map failed";
            case VK_ERROR_LAYER_NOT_PRESENT: return "Layer not present";
            case VK_ERROR_EXTENSION_NOT_PRESENT: return "Extension not present";
            case VK_ERROR_FEATURE_NOT_PRESENT: return "Feature not present";
            case VK_ERROR_INCOMPATIBLE_DRIVER: return "Incompatible driver";
            case VK_ERROR_TOO_MANY_OBJECTS: return "Too many objects";
            case VK_ERROR_FORMAT_NOT_SUPPORTED: return "Format not supported";

                // kronos extensions
            case VK_ERROR_SURFACE_LOST_KHR: return "[KHR] Surface lost";
            case VK_ERROR_NATIVE_WINDOW_IN_USE_KHR: return "[KHR] Native window in use";
            case VK_SUBOPTIMAL_KHR: return "[KHR] Suboptimal";
            case VK_ERROR_OUT_OF_DATE_KHR: return "[KHR] Out of date";
            case VK_ERROR_INCOMPATIBLE_DISPLAY_KHR: return "[KHR] Incompatible display";
            case VK_ERROR_VALIDATION_FAILED_EXT: return "[KHR] Validation failed";

                // NV extensions
            case VK_ERROR_INVALID_SHADER_NV: return "[NV] Invalid shader";

            default: return "<<unknown>>";
        }
    }

    VulkanAPIFailure::VulkanAPIFailure(VkResult res, const char message[])
        : Exceptions::BasicLabel("%s [%s, %i]", message, AsString(res), res) {}
}}
