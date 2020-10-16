Shader "StdPipeline/Defer Lighting" {
	HLSL : "6d06a621-aa2a-439d-aac6-eba6cd1d6f20"
	RootSignature {
		SRV[3] : 0
		SRV[1] : 3
		SRV[3] : 4
		SRV[2] : 7
		CBV : 0
		CBV : 1
	}
	Pass (VS, PS) {}
}
