#pragma once

#include <DirectXMath.h>
#include <cassert>
#include <initializer_list>
#include <type_traits>
#include <cstring> // for memcpy
#include <cmath>

namespace Detail {
	template <typename T, int N>
	struct VectorStorage {
		T data[N];
	};

	template <typename T>
	struct VectorStorage<T, 2> {
		union {
			T data[2];
			struct { T x, y; };
			struct { T u, v; };
		};
	};

	template <typename T>
	struct VectorStorage<T, 3> {
		union {
			T data[3];
			struct { T x, y, z; };
			struct { T r, g, b; };
		};
	};

	template <typename T>
	struct VectorStorage<T, 4> {
		union {
			T data[4];
			struct { T x, y, z, w; };
			struct { T r, g, b, a; };
		};
	};
}

// 固定長配列クラス (ベクトル)
template <typename T, int N>
class Vector : public Detail::VectorStorage<T, N> {
public:
	constexpr Vector() : Detail::VectorStorage<T, N>() {
		for (int i = 0; i < N; ++i) this->data[i] = static_cast<T>(0);
	}

	constexpr Vector(std::initializer_list<T> list) : Detail::VectorStorage<T, N>() {
		int i = 0;
		for (auto it = list.begin(); it != list.end() && i < N; ++it, ++i) {
			this->data[i] = *it;
		}
	}

	constexpr T& operator[](int index) {
		assert(index >= 0 && index < N);
		return this->data[index];
	}
	constexpr const T& operator[](int index) const {
		assert(index >= 0 && index < N);
		return this->data[index];
	}
	
	// ポインタへのキャスト
	constexpr operator T*() { return this->data; }
	constexpr operator const T*() const { return this->data; }

	//--- 演算子オーバーロード
	constexpr Vector operator+() const { return *this; }
	constexpr Vector operator-() const {
		Vector v;
		for (int i = 0; i < N; ++i) v.data[i] = -this->data[i];
		return v;
	}
	constexpr Vector& operator+=(const Vector& v) {
		for (int i = 0; i < N; ++i) this->data[i] += v.data[i];
		return *this;
	}
	constexpr Vector& operator-=(const Vector& v) {
		for (int i = 0; i < N; ++i) this->data[i] -= v.data[i];
		return *this;
	}
	constexpr Vector& operator*=(T s) {
		for (int i = 0; i < N; ++i) this->data[i] *= s;
		return *this;
	}
	constexpr Vector& operator*=(const Vector& v) {
		for (int i = 0; i < N; ++i) this->data[i] *= v.data[i];
		return *this;
	}
	constexpr Vector& operator/=(T s) {
		if (s == static_cast<T>(0)) return *this;
		for (int i = 0; i < N; ++i) this->data[i] /= s;
		return *this;
	}

	// 正規化
	Vector Normalized() const {
		T len2 = 0;
		for (int i = 0; i < N; ++i) len2 += this->data[i] * this->data[i];
		T len = std::sqrt(len2);
		if (len == 0) return *this;
		Vector v = *this;
		return v /= len;
	}

	// 長さ
	T Magnitude() const {
		T len2 = 0;
		for (int i = 0; i < N; ++i) len2 += this->data[i] * this->data[i];
		return std::sqrt(len2);
	}

	// 長さの二乗
	T MagnitudeSq() const {
		T len2 = 0;
		for (int i = 0; i < N; ++i) len2 += this->data[i] * this->data[i];
		return len2;
	}
};

// Vector<float, 4> の特殊化
template <>
class alignas(16) Vector<float, 4> {
public:
	union {
		DirectX::XMVECTOR v;
		float data[4];
		struct { float x, y, z, w; };
	};

	constexpr Vector() : data{ 0.0f, 0.0f, 0.0f, 0.0f } {}
	constexpr Vector(float _x, float _y, float _z, float _w = 0.0f)
		: data{ _x, _y, _z, _w } {
	}
	Vector(DirectX::XMVECTOR _v) { v = _v; }
	Vector(std::initializer_list<float> list) {
		float arr[4] = { 0,0,0,0 };
		int i = 0;
		for (auto it = list.begin(); it != list.end() && i < 4; ++it, ++i) {
			arr[i] = *it;
		}
		v = DirectX::XMLoadFloat4(reinterpret_cast<const DirectX::XMFLOAT4*>(arr));
	}

	float& operator[](int index) {
		assert(index >= 0 && index < 4);
		return data[index];
	}
	const float& operator[](int index) const {
		assert(index >= 0 && index < 4);
		return data[index];
	}

	operator float* () { return data; }
	operator const float* () const { return data; }

	operator DirectX::XMVECTOR() const { return v; }
	Vector& operator=(const DirectX::XMVECTOR& _v) { v = _v; return *this; }

	//--- 演算子オーバーロード
	constexpr Vector operator+() const { return *this; }
	Vector operator-() const { return DirectX::XMVectorNegate(v); }
	Vector& operator+=(const Vector& rhs) { v = DirectX::XMVectorAdd(v, rhs.v); return *this; }
	Vector& operator-=(const Vector& rhs) { v = DirectX::XMVectorSubtract(v, rhs.v); return *this; }
	Vector& operator*=(float s) { v = DirectX::XMVectorScale(v, s); return *this; }
	Vector& operator*=(const Vector& rhs) { v = DirectX::XMVectorMultiply(v, rhs.v); return *this; }
	Vector& operator/=(float s) {
		if (s == 0.0f) return *this;
		v = DirectX::XMVectorScale(v, 1.0f / s);
		return *this;
	}

	// 長さ
	float Magnitude() const {
		return DirectX::XMVectorGetX(DirectX::XMVector4Length(v));
	}

	// 長さの二乗
	float MagnitudeSq() const {
		return DirectX::XMVectorGetX(DirectX::XMVector4LengthSq(v));
	}
};

// Vector<float, 3> の特殊化 (XMVECTORラッパー)
template <>
class alignas(16) Vector<float, 3> {
public:
	union {
		DirectX::XMVECTOR v;
		float data[4]; // SIMD用に4要素確保
		struct { float x, y, z; };
	};

	constexpr Vector() : data{ 0.0f, 0.0f, 0.0f, 0.0f } {}
	constexpr Vector(float _x, float _y, float _z)
		: data{ _x, _y, _z, 0.0f } {
	}
	Vector(DirectX::XMVECTOR _v) { v = _v; }
	Vector(std::initializer_list<float> list) {
		float arr[4] = { 0,0,0,0 };
		int i = 0;
		for (auto it = list.begin(); it != list.end() && i < 3; ++it, ++i) {
			arr[i] = *it;
		}
		v = DirectX::XMLoadFloat4(reinterpret_cast<const DirectX::XMFLOAT4*>(arr));
	}

	float& operator[](int index) {
		assert(index >= 0 && index < 3);
		return data[index];
	}
	const float& operator[](int index) const {
		assert(index >= 0 && index < 3);
		return data[index];
	}

	operator float* () { return data; }
	operator const float* () const { return data; }

	operator DirectX::XMVECTOR() const { return v; }
	Vector& operator=(const DirectX::XMVECTOR& _v) { v = _v; return *this; }

	//--- 演算子オーバーロード
	constexpr Vector operator+() const { return *this; }
	Vector operator-() const { return DirectX::XMVectorNegate(v); }
	Vector& operator+=(const Vector& rhs) { v = DirectX::XMVectorAdd(v, rhs.v); return *this; }
	Vector& operator-=(const Vector& rhs) { v = DirectX::XMVectorSubtract(v, rhs.v); return *this; }
	Vector& operator*=(float s) { v = DirectX::XMVectorScale(v, s); return *this; }
	Vector& operator*=(const Vector& rhs) { v = DirectX::XMVectorMultiply(v, rhs.v); return *this; }
	Vector& operator/=(float s) {
		if (s == 0.0f) return *this;
		v = DirectX::XMVectorScale(v, 1.0f / s);
		return *this;
	}

	// 正規化
	Vector Normalized() const {
		return DirectX::XMVector3Normalize(v);
	}

	// 長さ
	float Magnitude() const {
		return DirectX::XMVectorGetX(DirectX::XMVector3Length(v));
	}

	// 長さの二乗
	float MagnitudeSq() const {
		return DirectX::XMVectorGetX(DirectX::XMVector3LengthSq(v));
	}
};

//--- 汎用演算子 (Vector<T, N>)
template <typename T, int N>
constexpr Vector<T, N> operator+(const Vector<T, N>& v1, const Vector<T, N>& v2) {
	Vector<T, N> v = v1; return v += v2;
}
template <typename T, int N>
constexpr Vector<T, N> operator-(const Vector<T, N>& v1, const Vector<T, N>& v2) {
	Vector<T, N> v = v1; return v -= v2;
}
template <typename T, int N>
constexpr Vector<T, N> operator*(const Vector<T, N>& v1, T s) {
	Vector<T, N> v = v1; return v *= s;
}
template <typename T, int N>
constexpr Vector<T, N> operator*(T s, const Vector<T, N>& v1) {
	Vector<T, N> v = v1; return v *= s;
}
template <typename T, int N>
constexpr Vector<T, N> operator*(const Vector<T, N>& v1, const Vector<T, N>& v2) {
	Vector<T, N> v = v1; return v *= v2;
}
template <typename T, int N>
constexpr Vector<T, N> operator/(const Vector<T, N>& v1, T s) {
	Vector<T, N> v = v1; return v /= s;
}

//--- SIMD演算子 (Vector<float, 4>)
inline Vector<float, 4> operator+(const Vector<float, 4>& v1, const Vector<float, 4>& v2) {
	return Vector<float, 4>(DirectX::XMVectorAdd(v1.v, v2.v));
}
inline Vector<float, 4> operator-(const Vector<float, 4>& v1, const Vector<float, 4>& v2) {
	return Vector<float, 4>(DirectX::XMVectorSubtract(v1.v, v2.v));
}
inline Vector<float, 4> operator*(const Vector<float, 4>& v, float s) {
	return Vector<float, 4>(DirectX::XMVectorScale(v.v, s));
}
inline Vector<float, 4> operator*(float s, const Vector<float, 4>& v) {
	return Vector<float, 4>(DirectX::XMVectorScale(v.v, s));
}
// ベクトル同士の乗算（成分ごとの積）
inline Vector<float, 4> operator*(const Vector<float, 4>& v1, const Vector<float, 4>& v2) {
	return Vector<float, 4>(DirectX::XMVectorMultiply(v1.v, v2.v));
}
inline Vector<float, 4> operator/(const Vector<float, 4>& v, float s) {
	return Vector<float, 4>(DirectX::XMVectorScale(v.v, 1.0f / s));
}

//--- SIMD演算子 (Vector<float, 3>)
inline Vector<float, 3> operator+(const Vector<float, 3>& v1, const Vector<float, 3>& v2) {
	return Vector<float, 3>(DirectX::XMVectorAdd(v1.v, v2.v));
}
inline Vector<float, 3> operator-(const Vector<float, 3>& v1, const Vector<float, 3>& v2) {
	return Vector<float, 3>(DirectX::XMVectorSubtract(v1.v, v2.v));
}
inline Vector<float, 3> operator*(const Vector<float, 3>& v, float s) {
	return Vector<float, 3>(DirectX::XMVectorScale(v.v, s));
}
inline Vector<float, 3> operator*(float s, const Vector<float, 3>& v) {
	return Vector<float, 3>(DirectX::XMVectorScale(v.v, s));
}
// ベクトル同士の乗算（成分ごとの積）
inline Vector<float, 3> operator*(const Vector<float, 3>& v1, const Vector<float, 3>& v2) {
	return Vector<float, 3>(DirectX::XMVectorMultiply(v1.v, v2.v));
}
inline Vector<float, 3> operator/(const Vector<float, 3>& v, float s) {
	return Vector<float, 3>(DirectX::XMVectorScale(v.v, 1.0f / s));
}

//--- 数学関数
// 内積 (Generic)
template <typename T, int N>
constexpr T Dot(const Vector<T, N>& v1, const Vector<T, N>& v2) {
	T ret = 0;
	for (int i = 0; i < N; ++i) ret += v1[i] * v2[i];
	return ret;
}

// 内積 (Vector3)
inline float Dot(const Vector<float, 3>& v1, const Vector<float, 3>& v2) {
	return DirectX::XMVectorGetX(DirectX::XMVector3Dot(v1.v, v2.v));
}

// 内積 (Vector4)
inline float Dot(const Vector<float, 4>& v1, const Vector<float, 4>& v2) {
	return DirectX::XMVectorGetX(DirectX::XMVector4Dot(v1.v, v2.v));
}

// 外積 (Vector3)
inline Vector<float, 3> Cross(const Vector<float, 3>& v1, const Vector<float, 3>& v2) {
	return Vector<float, 3>(DirectX::XMVector3Cross(v1.v, v2.v));
}

// 型エイリアス
using Vector2 = Vector<float, 2>;
using Vector3 = Vector<float, 3>;
using Vector4 = Vector<float, 4>;