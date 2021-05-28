#pragma once
#include <glm/glm.hpp>
#include "minmax_tracker.hpp"
#include "ray.hpp"

namespace bu::rt {

struct aabb
{
	glm::vec3 min;
	glm::vec3 max;

	inline void add_point(const glm::vec3 &p);
	inline void add_aabb(const aabb &box);

	inline bool test_ray(const rt::ray &r, float &t) const;
	inline bool check_overlap(const aabb &other) const;
	inline glm::vec3 get_dimensions() const;
	inline glm::vec3 get_center() const;
	inline float get_area() const;
};

void aabb::add_point(const glm::vec3 &p)
{
	min = glm::min(min, p);
	max = glm::max(max, p);
}

void aabb::add_aabb(const aabb &box)
{
	min = glm::min(min, box.min);
	max = glm::max(max, box.max);
}

inline bool aabb::test_ray(const rt::ray &r, float &t) const
{
	glm::vec3 a{(min - r.origin) / r.direction};
	glm::vec3 b{(max - r.origin) / r.direction};

	float tmin = std::max(std::max(std::min(a.x, b.x), std::min(a.y, b.y)), std::min(a.z, b.z));
	float tmax = std::min(std::min(std::max(a.x, b.x), std::max(a.y, b.y)), std::max(a.z, b.z));

	if (tmax < 0 || tmin > tmax)
		return false;
	else
	{
		t = tmin;
		return true;
	}
}

inline bool aabb::check_overlap(const aabb &other) const
{
	return min.x < other.max.x && max.x > other.min.x
		&& min.y < other.max.y && max.y > other.min.y
		&& min.z < other.max.z && max.z > other.min.z;
}

glm::vec3 aabb::get_dimensions() const
{
	return max - min;
}

glm::vec3 aabb::get_center() const
{
	return glm::vec3{max + min} * 0.5f;
}

float aabb::get_area() const
{
	auto d = get_dimensions();
	return (d.x * d.y + d.x * d.z + d.y * d.z) * 2.f;
}

class aabb_collection
{
public:
	void add(const aabb &box)
	{
		x.add(box.min.x);
		x.add(box.max.x);
		y.add(box.min.y);
		y.add(box.max.y);
		z.add(box.min.z);
		z.add(box.max.z);
	}

	void remove(const aabb &box)
	{
		x.remove(box.min.x);
		x.remove(box.max.x);
		y.remove(box.min.y);
		y.remove(box.max.y);
		z.remove(box.min.z);
		z.remove(box.max.z);
	}

	glm::vec3 get_min() const
	{
		return {x.get_min(), y.get_min(), z.get_min()};
	}

	glm::vec3 get_max() const
	{
		return {x.get_max(), y.get_max(), z.get_max()};
	}

	aabb get_aabb() const
	{
		return {get_min(), get_max()};
	}

	unsigned int size() const
	{
		return x.size();
	}

private:
	minmax_tracker x;
	minmax_tracker y;
	minmax_tracker z;
};

aabb triangle_aabb(const bu::rt::triangle &t);
aabb triangles_aabb(const bu::rt::triangle *arr, unsigned int size);

}