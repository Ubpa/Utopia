Shader "StdPipeline/ATrous" {
	HLSL : "a9f8e983-71a2-43ba-9362-f058ebd983d8"
	RootSignature {
		SRV[2] : 0 // gRTS, gRTU
		SRV[1] : 2 // gGBuffer1
		SRV[1] : 3 // gLinearZ
		CBV : 0
		CBV : 1
	}
	Pass (VS, PS) {}
}
