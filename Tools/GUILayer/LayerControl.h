// Copyright 2015 XLGAMES Inc.
//
// Distributed under the MIT License (See
// accompanying file "LICENSE" or the website
// http://www.opensource.org/licenses/mit-license.php)

#pragma once

#include "EngineControl.h"

namespace GUILayer 
{
    class LayerControlPimpl;

    ref class ModelVisSettings;

	public ref class LayerControl : public EngineControl
	{
	public:
        void SetupDefaultVis(ModelVisSettings^ settings);

		LayerControl();
		~LayerControl();
    protected:
        virtual void Render(RenderCore::IThreadContext&, IWindowRig&) override;
        clix::auto_ptr<LayerControlPimpl> _pimpl;
	};
}

