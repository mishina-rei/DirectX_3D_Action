#pragma once

#include <DirectXMath.h>
#include "Vector.h"
#include <cmath>

class alignas(16) Quaternion {
public:
	union {
		DirectX::XMVECTOR v;
		struct { float x, y, z, w; };
		float data[4];
	};

	//--- コンストラクタ
	// デフォルトは単位元 (0, 0, 0, 1)
	constexpr Quaternion() : x(0.0f), y(0.0f), z(0.0f), w(1.0f) {}
	constexpr Quaternion(float _x, float _y, float _z, float _w)
		: x(_x), y(_y), z(_z), w(_w) {}
	Quaternion(float _x, float _y, float _z) { *this = FromRotation(_x, _y, _z); }
	Quaternion(DirectX::XMVECTOR _v) { v = _v; }

	//--- 代入演算子
	Quaternion& operator=(const DirectX::XMVECTOR& _v) { v = _v; return *this; }

	//--- キャスト
	operator DirectX::XMVECTOR() const { return v; }

	//--- 単項演算子
	constexpr Quaternion operator+() const { return *this; }
	Quaternion operator-() const { return DirectX::XMVectorNegate(v); }

	//--- 二項演算子 (代入)
	Quaternion& operator+=(const Quaternion& rhs) { v = DirectX::XMVectorAdd(v, rhs.v); return *this; }
	Quaternion& operator-=(const Quaternion& rhs) { v = DirectX::XMVectorSubtract(v, rhs.v); return *this; }
	Quaternion& operator*=(const Quaternion& rhs) { v = DirectX::XMQuaternionMultiply(v, rhs.v); return *this; }
	Quaternion& operator*=(float s) { v = DirectX::XMVectorScale(v, s); return *this; }
	Quaternion& operator/=(float s) { v = DirectX::XMVectorScale(v, 1.0f / s); return *this; }

	//--- 便利関数
	// 単位元
	static Quaternion Identity() { return Quaternion(0.0f, 0.0f, 0.0f, 1.0f); }
	
	// 軸回転
	static Quaternion RotationAxis(const Vector<float, 3>& axis, float angle) {
		return DirectX::XMQuaternionRotationAxis(axis, angle);
	}
	
	// オイラー角から作成
	static Quaternion FromRotation(float pitch, float yaw, float roll) {
		pitch = DirectX::XMConvertToRadians(pitch);
		yaw = DirectX::XMConvertToRadians(yaw);
		roll = DirectX::XMConvertToRadians(roll);	
		return DirectX::XMQuaternionRotationRollPitchYaw(pitch, yaw, roll);
	}

	// オイラー角から作成(Vectorから)
	static Quaternion FromRotation(Vector<float, 3> angles) {
		angles.x = DirectX::XMConvertToRadians(angles.x);
		angles.y = DirectX::XMConvertToRadians(angles.y);	
		angles.z = DirectX::XMConvertToRadians(angles.z);
		return DirectX::XMQuaternionRotationRollPitchYawFromVector(angles);
	}

	// 回転行列から作成
	static Quaternion RotationMatrix(const DirectX::XMMATRIX& mat) {
		return DirectX::XMQuaternionRotationMatrix(mat);
	}

	// 球面線形補間 (Slerp)
	static Quaternion Slerp(const Quaternion& q1, const Quaternion& q2, float t) {
		return DirectX::XMQuaternionSlerp(q1, q2, t);
	}

	// 共役
	Quaternion Conjugate() const {
		return DirectX::XMQuaternionConjugate(v);
	}

	// 逆元
	Quaternion Inverse() const {
		return DirectX::XMQuaternionInverse(v);
	}

	// 正規化
	Quaternion Normalize() const {
		return DirectX::XMQuaternionNormalize(v);
	}

	// ベクトル回転
	Vector<float, 3> Rotate(const Vector<float, 3>& vec) const {
		return DirectX::XMVector3Rotate(vec, v);
	}

	// オイラー角の数値を返す
	Vector3 ToEuler() {
		Vector3 angles;

		// Pitch (X)
		float sinp = 2.0f * (w * x - y * z);
		if (std::abs(sinp) >= 1.0f)
			angles.x = std::copysign(DirectX::XM_PI / 2.0f, sinp);
		else
			angles.x = std::asin(sinp);

		// Yaw (Y)
		float siny = 2.0f * (w * y + x * z);
		float cosy = 1.0f - 2.0f * (x * x + y * y);
		angles.y = std::atan2(siny, cosy);

		// Roll (Z)
		float sinr = 2.0f * (w * z + x * y);
		float cosr = 1.0f - 2.0f * (x * x + z * z);
		angles.z = std::atan2(sinr, cosr);

		// ラジアンから度数法に変換
		angles.x = DirectX::XMConvertToDegrees(angles.x);
		angles.y = DirectX::XMConvertToDegrees(angles.y);
		angles.z = DirectX::XMConvertToDegrees(angles.z);

		return angles;
	}
};

//--- 二項演算子 (グローバル)
inline Quaternion operator+(const Quaternion& q1, const Quaternion& q2) {
	return DirectX::XMVectorAdd(q1.v, q2.v);
}
inline Quaternion operator-(const Quaternion& q1, const Quaternion& q2) {
	return DirectX::XMVectorSubtract(q1.v, q2.v);
}
inline Quaternion operator*(const Quaternion& q1, const Quaternion& q2) {
	return DirectX::XMQuaternionMultiply(q1.v, q2.v);
}
inline Quaternion operator*(const Quaternion& q, float s) {
	return DirectX::XMVectorScale(q.v, s);
}
inline Quaternion operator*(float s, const Quaternion& q) {
	return DirectX::XMVectorScale(q.v, s);
}

// ベクトル回転 (q * v)
inline Vector<float, 3> operator*(const Quaternion& q, const Vector<float, 3>& v) {
	return DirectX::XMVector3Rotate(v, q.v);
}

// ベクトル回転 (v * q)
inline Vector<float, 3> operator*(const Vector<float, 3>& v, const Quaternion& q) {
	return DirectX::XMVector3Rotate(v, q.v);
}
