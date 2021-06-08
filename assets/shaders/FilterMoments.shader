Shader "StdPipeline/FilterMoments" {
	HLSL : "44c9274b-7d39-4806-9143-931b39916535"
	RootSignature {
		SRV[4] : 0 // gRTS, gRTU, gMoments, gHistoryLength
		SRV[1] : 4 // gGBuffer1
		SRV[1] : 5 // gLinearZ
		CBV    : 0 // camera
	}
	Pass (VS, PS) {}
}
