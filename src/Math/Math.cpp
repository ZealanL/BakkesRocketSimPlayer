#include "Math.h"

Quat Math::RotMatToQuat(Vector forward, Vector right, Vector up) {
	// Copied from BulletPhysics btMatrix3x3::getRotation()

	float m_el[3][3] = {
		{ forward.X, right.X, up.X },
		{ forward.Y, right.Y, up.Y },
		{ forward.Z, right.Z, up.Z }
	};

	float trace = m_el[0][0] + m_el[1][1] + m_el[2][2];

	float temp[4];

	if (trace > 0.0f) {
		float s = sqrt(trace + 1.0f);
		temp[3] = (s * 0.5f);
		s = 0.5f / s;

		temp[0] = ((m_el[2][1] - m_el[1][2]) * s);
		temp[1] = ((m_el[0][2] - m_el[2][0]) * s);
		temp[2] = ((m_el[1][0] - m_el[0][1]) * s);
	} else {
		int i = m_el[0][0] < m_el[1][1] ? (m_el[1][1] < m_el[2][2] ? 2 : 1) : (m_el[0][0] < m_el[2][2] ? 2 : 0);
		int j = (i + 1) % 3;
		int k = (i + 2) % 3;

		float s = sqrtf(m_el[i][i] - m_el[j][j] - m_el[k][k] + 1.0f);
		temp[i] = s * 0.5f;
		s = 0.5f / s;

		temp[3] = (m_el[k][j] - m_el[j][k]) * s;
		temp[j] = (m_el[j][i] + m_el[i][j]) * s;
		temp[k] = (m_el[k][i] + m_el[i][k]) * s;
	}

	return Quat(temp[3], temp[0], temp[1], temp[2]);
}

float Math::Interp(float to, float from, float ratio) {
	return from + (to - from) * ratio;
}

Vector Math::Interp(Vector from, Vector to, float ratio) {
	if (ratio == 0) {
		return from;
	} else {
		Vector delta = to - from;
		return from + delta * ratio;
	}
}

float QDot(Quat a, Quat b) {
	return
		a.X * b.X +
		a.Y * b.Y +
		a.Z * b.Z +
		a.W * b.W;
}

Quat Math::Interp(Quat from, Quat to, float ratio) {
	// From btQuaternion::slerp

	const float magnitude = abs(QDot(from, from) * QDot(to, to));

	const float product = (QDot(from, to)) / magnitude;
	const float absproduct = abs(product);

	if (absproduct < (1.0f - 1e-7)) {
		// Take care of long angle case see http://en.wikipedia.org/wiki/Slerp
		const float theta = acos(absproduct);
		const float d = sin(theta);

		const float sign = (product < 0) ? -1 : 1;
		const float s0 = sin((1 - ratio) * theta) / d;
		const float s1 = sin(sign * ratio * theta) / d;

		return Quat(
			from.W * s0 + to.W * s1,
			from.X * s0 + to.X * s1,
			from.Y * s0 + to.Y * s1,
			from.Z * s0 + to.Z * s1
		);
	} else {
		return from;
	}
}