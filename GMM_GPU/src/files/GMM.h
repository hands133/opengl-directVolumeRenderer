#pragma once

#include <glm/glm.hpp>

#include <vector>

// no weights
struct GaussianFunc
{
	double mean = 0.0;
	double var = 1.0;

	inline double operator()(double x) const;
};

// simple kernel function of gaussian function
// NOTE: no variance matrix
struct GaussianKernel
{
	GaussianKernel() : weight(0.0f) {}
	GaussianKernel(double w, const glm::dvec3& m, const glm::dvec3& v);
	double weight;
	//3D - Gaussian kernel
	struct funcSet {
		GaussianFunc x, y, z;
	} funcs;

	double GaussianKernel::operator()(const glm::dvec3& samplePoint) const;
};

// One GMM in each bin could contain some Gaussian kernel functions
class GMMBin
{
public:
	GMMBin() : m_occupyKernels(0), m_prob(0.0) {}
	GMMBin(const GMMBin& bin);
	~GMMBin() {};

	void SetProb(double w) { m_prob = w; }
	void SetKernelWeight(double w, int index);
	void SetKernelMeans(const glm::dvec3& means, int index);
	void SetKernelVars(const glm::dvec3& vars, int index);

	void Push(const GaussianKernel& kernel)
	{
		m_kernels[m_occupyKernels] = kernel;
		m_occupyKernels++;
	}

	double GetProb() const { return m_prob; }
	int GetKernelNum() const { return m_occupyKernels; }
	static int GetKernelCapacity() { return m_copacityKernels; }
	const GaussianKernel& GetKernel(int index) const { return m_kernels[index]; }

	double operator()(const glm::dvec3& samplePoint) const;
	double EvalWithoutProb(const glm::dvec3& samplePoint) const;
	GMMBin& operator=(const GMMBin& bin);

private:
	static int m_copacityKernels;
	int m_occupyKernels;
	double m_prob;
	GaussianKernel m_kernels[4];
};