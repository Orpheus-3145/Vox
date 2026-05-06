#include "tests.hpp"

static bool	check(const vec3& a, const vec3& b)
{
	return	std::abs(a.x - b.x) < IMPRECISION_TOLERANCE &&
			std::abs(a.y - b.y) < IMPRECISION_TOLERANCE &&
			std::abs(a.z - b.z) < IMPRECISION_TOLERANCE;
}

static int	testLength(const vec3& vector, const glm::vec3& glmvector)
{
	float	result = vector.length();
	float	expectedResult = glm::length(glmvector);

	if (std::abs(result - expectedResult) < IMPRECISION_TOLERANCE)
	{
		std::cout << GREEN << "[OK]\n" << RESET;
		return 0;
	}
	else
	{
		std::cout << RED << "[FAIL]\n" << RESET;
		std::cout << "Expected: " << expectedResult;
		std::cout << ", Got: " << result;
		std::cout << std::endl;
	}
	return 1;
}

static int	testNormalize(const vec3& vector, const glm::vec3& expectedResult)
{
	vec3	unitVector = vector.normalized();
	vec3	expected = vec3{expectedResult.x, expectedResult.y, expectedResult.z};

	if (check(unitVector, expected) == true)
	{
		std::cout << GREEN << "[OK]\n";
		return 0;
	}
	else
	{
		std::cout << RED << "[FAIL]\n" << RESET;
		std::cout << "expected: " << expected;
		std::cout << ", got: " << unitVector;
		std::cout << std::endl;
		return 1;
	}
}


static int	testVectorNormalize()
{
	int	failures = 0;

	std::cout << RESET << "Normalize vector test:" << std::endl;
	failures += testNormalize(vec3(1, 0, 0), glm::normalize(glm::vec3(1, 0, 0)));
	failures += testNormalize(vec3(0, 1, 0), glm::normalize(glm::vec3(0, 1, 0)));
	failures += testNormalize(vec3(6, 2, 1), glm::normalize(glm::vec3(6, 2, 1)));
	failures += testNormalize(vec3(-1, 0, 100), glm::normalize(glm::vec3(-1, 0, 100)));
	failures += testNormalize(vec3(4000, -7000, 9999), glm::normalize(glm::vec3(4000, -7000, 9999)));
	return failures;
}

static int	testVectorLength()
{
	int	failures = 0;

	std::cout << RESET << "Vector length test:\n";
	failures += testLength(vec3(1.f, 1.f, 1.f), glm::vec3(1.f, 1.f, 1.f));
	failures += testLength(vec3(0.57f, 0.57f, 0.57f), glm::vec3(0.57f, 0.57f, 0.57f));
	failures += testLength(vec3(-1.f, -4.f, 5.5f), glm::vec3(-1.f, -4.f, 5.5f));
	failures += testLength(vec3(-1.f, 0.f, 100.f), glm::vec3(-1.f, 0.f, 100.f));
	failures += testLength(vec3(4000.f, -7000.f, 9999.f), glm::vec3(4000.f, -7000.f, 9999.f));
	return failures;
}

int	runVectorTests()
{
	int	failures = 0;

	std::cout << "Vector data size tests:\n";
	failures += compareSizes(sizeof(vec2), sizeof(glm::vec2));
	failures += compareSizes(sizeof(vec3), sizeof(glm::vec3));
	failures += compareSizes(sizeof(vec4), sizeof(glm::vec4));
	failures += testVectorLength();
	failures += testVectorNormalize();

	return failures;
}
