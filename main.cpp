#include "Win32_GLAppUtil.h"

bool isVisible = true;
Scene* roomScene = nullptr;

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
	
	// *** Frame rendering steps:
	// 1) obtain predicted eye poses based on the headset tracking pose,
	// 2) render the view for each eye,
	// 3) and finally submit eye textures to compositor through ovr_SubmitFrame
	// Getting ready for frame rendering.	

	// Initialize some data structures that can be shared across frames.
	
	ovrEyeRenderDesc eyeRenderDesc[2]; // one for each eye.
	ovrVector3f	hmdToEyeViewOffset[2];

	// eye render description
	eyeRenderDesc[0] = ovr_GetRenderDesc(session, ovrEye_Left, hmdDesc.DefaultEyeFov[0]);
	eyeRenderDesc[1] = ovr_GetRenderDesc(session, ovrEye_Right, hmdDesc.DefaultEyeFov[1]);
	hmdToEyeViewOffset[0] = eyeRenderDesc[0].HmdToEyeOffset; // used  later for adjusting eye separation.
	hmdToEyeViewOffset[1] = eyeRenderDesc[1].HmdToEyeOffset; // same as above.

	// init our single full screen fov layer (dual-eye layer)

	ovrLayerEyeFov layer;
	layer.Header.Type		= ovrLayerType_EyeFov;
	layer.Header.Flags		= 0;
	// Setting same large texture
	layer.ColorTexture[0]	= textureSwapChain; 
	layer.ColorTexture[1]	= textureSwapChain;
	layer.Fov[0]			= eyeRenderDesc[0].Fov;
	layer.Fov[1]			= eyeRenderDesc[1].Fov;
	// Set the view port in a way that left eye takes the left part of the
	// the big texture, and the right eye takes the right part of the big texture
	// things start to make more sense why we come up with a one big large 
	// having tex0.w + tex1.w in the very beginning
	layer.Viewport[0] = Recti(0, 0,					bufferSize.w / 2, bufferSize.h);
	layer.Viewport[1] = Recti(bufferSize.w / 2, 0,	bufferSize.w / 2, bufferSize.h);

	// Get both eye poses for rendering loop
	while (1) {
		double displayMidpointSeconds = ovr_GetPredictedDisplayTime(session, 0);
		ovrTrackingState hmdState = ovr_GetTrackingState(session, displayMidpointSeconds, ovrTrue);
		// CalcEyePoses output the result to layer.RenderPos
		ovr_CalcEyePoses(hmdState.HeadPose.ThePose, hmdToEyeViewOffset, layer.RenderPose);

		if (isVisible) {
			// Get next availbale index of the texture swap chain.
			int currentIndex = 0;
			ovr_GetTextureSwapChainCurrentIndex(session, textureSwapChain, &currentIndex);
			/*
			for (int eye = 0; eye < 2; ++eye)
			{
				//eyeRenderTexture[eye]->SetAndClearRenderSurface(eyeDepthBuffer[eye]);
				// Get view and projection matrices for the Rift camera
				Vector3f pos = originPos + originRot.Transform(layer.RenderPose[eye].Position);
				Matrix4f rot = originRot * Matrix4f(layer.RenderPose[eye].Orientation);

				Vector3f finalUp = rot.Transform(Vector3f(0, 1, 0));
				Vector3f finalForward = rot.Transform(Vector3f(0, 0, -1));
				Matrix4f view = Matrix4f::LookAtRH(pos, pos + finalForward, finalUp);

				Matrix4f proj = ovrMatrix4f_Projection(layer.Fov[eye], 0.2f, 1000.0f, 0);
				// Render the scene for this eye.
				//DIRECTX.SetViewport(layer.Viewport[eye]);
				roomScene.Render(proj * view, 1, 1, 1, 1, true);
			}
			*/
			

			// COmmit the changes to the texture swap chain.
			ovr_CommitTextureSwapChain(session, textureSwapChain);
		}
		ovrLayerHeader* layers = &layer.Header;
		ovrResult	result = ovr_SubmitFrame(session, 0, nullptr, &layers, 1);
		isVisible = (result == ovrSuccess);
		ovrErrorInfo errorInfo;
		ovr_GetLastErrorInfo(&errorInfo);
		int x = 0;
	}
	ovr_Destroy(session);
	ovr_Shutdown();
	return(0);
}