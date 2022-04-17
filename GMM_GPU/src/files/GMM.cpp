#include "GMM.h"

#include "tinyVR.h"
#include <algorithm>

GaussianKernel::GaussianKernel(double w, const glm::dvec3& m, const glm::dvec3& v)
{
	weight = w;
	funcs.x.mean = m.x;
	funcs.y.mean = m.y;
	funcs.z.mean = m.z;

	funcs.x.var = v.x;
	funcs.y.var = v.y;
	funcs.z.var = v.z;
}

inline double GaussianFunc::operator()(double x) const
{
	double P = 1.0;
	// NOTE : Notice var is ¦Ò^2
	P = 1.0 / (std::sqrt(2.0 * PI_D * var));
	P *= std::exp(-0.5 * (x - mean) * (x - mean) / var);
	return P;
}

double GaussianKernel::operator()(const glm::dvec3& samplePoint) const
{
	double P = 1.0;
	P *= funcs.x(samplePoint.x);
	P *= funcs.y(samplePoint.y);
	P *= funcs.z(samplePoint.z);
	return P * weight;
}

int GMMBin::m_copacityKernels = 4;

GMMBin::GMMBin(const GMMBin& bin)
{
	m_prob = bin.m_prob;

	std::copy(bin.m_kernels, bin.m_kernels + 4, m_kernels);

	m_occupyKernels = bin.m_occupyKernels;
}

void GMMBin::SetKernelWeight(double w, int index)
{
	m_kernels[index].weight = w;
}

void GMMBin::SetKernelMeans(const glm::dvec3& means, int index)
{
	m_kernels[index].funcs.x.mean = means.x;
	m_kernels[index].funcs.y.mean = means.y;
	m_kernels[index].funcs.z.mean = means.z;
}

void GMMBin::SetKernelVars(const glm::dvec3& vars, int index)
{
	m_kernels[index].funcs.x.var = vars.x;
	m_kernels[index].funcs.y.var = vars.y;
	m_kernels[index].funcs.z.var = vars.z;
}

double GMMBin::operator()(const glm::dvec3& samplePoint) const
{
	return EvalWithoutProb(samplePoint) * m_prob;
}

double GMMBin::EvalWithoutProb(const glm::dvec3& samplePoint) const
{
	double P = 0.0;
	for (int i = 0; i < m_occupyKernels; ++i)
		P += m_kernels[i](samplePoint);
	return P;
}

GMMBin& GMMBin::operator=(const GMMBin& bin)
{
	m_occupyKernels = 0;
	for (int i = 0; i < bin.m_occupyKernels; ++i)	Push(bin.m_kernels[i]);
	m_prob = bin.m_prob;

	return *this;
}
