Shader "StdPipeline/RTRepeoject" {
	HLSL : "5a963cf5-d261-453c-bcfc-f1e1d8a2eabe"
	RootSignature {
		SRV[6] : 0 // gPrevRTS, gPrevRTU, gPrevColorMoment, gMotion, gPrevLinearZ, gLinearZ
		UAV[4] : 0 // gRTS, gRTU, gColorMoment, gHistoryLength
		CBV    : 0 // camera
	}
	Pass (VS, PS) {}
}
