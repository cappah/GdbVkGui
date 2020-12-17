#pragma once

#include "Gui/GuiLayer.h"

#ifdef __cplusplus
extern "C"
{
#endif

	typedef struct LoadSettings
	{
		int m_MaxFileSz;
	} LoadSettings;

    void InitFrontend(const LoadSettings* settings);

    int DrawFrontend(void);

#ifdef __cplusplus
}
#endif
