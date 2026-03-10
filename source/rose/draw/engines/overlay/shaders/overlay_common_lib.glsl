#pragma once

float4x4 extract_matrix_packed_data(float4x4 mat, out float4 dataA, out float4 dataB) {
	const float div = 1.0f / 255.0f;
	int a = int(mat[0][3]);
	int b = int(mat[1][3]);
	int c = int(mat[2][3]);
	int d = int(mat[3][3]);
	dataA = float4(a & 0xFF, a >> 8, b & 0xFF, b >> 8) * div;
	dataB = float4(c & 0xFF, c >> 8, d & 0xFF, d >> 8) * div;
	mat[0][3] = mat[1][3] = mat[2][3] = 0.0f;
	mat[3][3] = 1.0f;
	return mat;
}
