#include "Win32_GLAppUtil.h"

int WINAPI WinMain(HINSTANCE hinst, HINSTANCE, LPSTR, int)
{
	// Initializes LibOVR, and the Rift
	ovrInitParams initParams = { ovrInit_RequestVersion, OVR_MINOR_VERSION, NULL, 0, 0 };
	ovrResult result = ovr_Initialize(&initParams);
	VALIDATE(OVR_SUCCESS(result), "Failed to initialize libOVR.");

	// check sdk version for fun.
	const char* sdkVersion = ovr_GetVersionString();

	// Setup the handle a Oculus HMD.
	ovrSession session;
	ovrGraphicsLuid luid; //local uid
	
	result = ovr_Create(&session, &luid);
	if (OVR_FAILURE(result)) {
		ovr_Shutdown();
		return -1;
	}

	ovrHmdDesc hmdDesc = ovr_GetHmdDesc(session);
	ovrSizei resolution = hmdDesc.Resolution;
	
	ovrTrackingState ts;
	//while (1) {
		ts = ovr_GetTrackingState(session, ovr_GetTimeInSeconds(), ovrTrue);
		//
	//}
	
	Sizei recommendedTex0Size = ovr_GetFovTextureSize(
		session,
		ovrEye_Left,
		hmdDesc.DefaultEyeFov[0],
		1.0f
	);

	Sizei recommendedTex1Size = ovr_GetFovTextureSize(
		session,
		ovrEye_Right,
		hmdDesc.DefaultEyeFov[1],
		1.0f //Pixels per display pixel
	);
	
	// using one shared texture, so have to make it large enought to fit both eye
	// renderings.
	Sizei bufferSize;
	bufferSize.w = recommendedTex0Size.w + recommendedTex1Size.w;
	bufferSize.h = OVRMath_Max(recommendedTex0Size.h, recommendedTex1Size.h);
	
	// create an empty texture swap chain (GL)
	ovrTextureSwapChain textureSwapChain = NULL;
	
	// a description of the texture swap chain.
	ovrTextureSwapChainDesc desc;
	desc.Type = ovrTexture_2D;
	desc.ArraySize = 1;
	desc.Format = OVR_FORMAT_B8G8R8A8_UNORM_SRGB;
	desc.Width = bufferSize.w;
	desc.Height = bufferSize.h;
	desc.MipLevels = 1;
	desc.SampleCount = 1;
	desc.StaticImage = ovrFalse;
	
	// create a texture swap chain with the description above.
	
	if (ovr_CreateTextureSwapChainGL(session, &desc, &textureSwapChain) == ovrSuccess) {
		unsigned int texId;
		ovr_GetTextureSwapChainBufferGL(session, textureSwapChain, 0, &texId);

		// Gotten the texture id (texID) generate by the texture swap chain API, which
		// means that we can just make changes to this texture, and the texture swap chain
		// will have access to it for later rendering.

		//glEnable(GL_FRAMEBUFFER_SRGB);
		glBindTexture(GL_TEXTURE_2D, texId);
		// do more to the texture with openGL codes.
		// ...
	}
	
	// Frame rendering steps:
	// 1) obtain predicted eye poses based on the headset tracking pose,
	// 2) render the view for each eye,
	// 3) and finally submit eye textures to compositor through ovr_SubmitFrame
	// Getting ready for frame rendering.	

	// Initialize some data structures that can be shared across frames.
	
	ovrEyeRenderDesc eyeRenderDesc[2]; // one for each eye.
	ovrVector3f	hmdToEyeViewOffset[2];
	
	eyeRenderDesc[0] = ovr_GetRenderDesc(session, ovrEye_Left, hmdDesc.DefaultEyeFov[0]);

	
	ovr_Destroy(session);
	ovr_Shutdown();
	return(0);
}