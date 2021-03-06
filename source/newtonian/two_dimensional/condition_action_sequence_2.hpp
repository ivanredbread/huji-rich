#ifndef CONDITION_ACTION_SEQUENCE2_HPP
#define CONDITION_ACTION_SEQUENCE2_HPP 1

#include "condition_action_sequence.hpp"
#include "spatial_reconstruction.hpp"
#include "../common/LagrangianHLLC.hpp"

//! \brief Second order flux calculator based on a series of conditions and actions
class ConditionActionSequence2 : public FluxCalculator
{
public:

	//! \brief Action taken to calculate flux
	class Action2
	{
	public:

		/*! \brief Calculates flux
		\param edge Interface between cells
		\param index The index of the edge
		\param tess Tessellation
		\param cells Computational cells
		\param eos Equation of state
		\param aux Auxiliary variable for assymetric problems (true means the relevant cell is on the left side, false mean right)
		\param edge_values The interpolated values at the edge
		\param edge_velocity Velocity of the edges
		\param res The flux given as output
		\param time The time
		\param tracerstickernames The names of the tracers and stickers
		*/
		virtual void operator()
			(const Edge& edge,
				const size_t index,
				const Tessellation& tess,
				const Vector2D& edge_velocity,
				const vector<ComputationalCell>& cells,
				const EquationOfState& eos,
				const bool aux,
				const pair<ComputationalCell, ComputationalCell> & edge_values,
				Extensive &res,
				double time,
				TracerStickerNames const& tracerstickernames) const = 0;

	  /*! \brief Return instance to initial state
	   */
		virtual void Reset(void) const {}

		virtual ~Action2(void);
	private:
	  //	  Action2(const Action2&);
	};

	/*! \brief Class constructor
	\param sequence Series of condition and action action pairs
	\param interp Interpolation
	\param sequence2 List of second order condition action sequences
	*/
	ConditionActionSequence2
		(const vector<pair<const ConditionActionSequence::Condition*, const ConditionActionSequence::Action*> >& sequence,
			const vector<pair<const ConditionActionSequence::Condition*, const ConditionActionSequence2::Action2*> >& sequence2,
			SpatialReconstruction const& interp);

	~ConditionActionSequence2(void);

	vector<Extensive> operator()
		(const Tessellation& tess,
			const vector<Vector2D>& edge_velocities,
			const vector<ComputationalCell>& cells,
			const vector<Extensive>& extensives,
			const CacheData& cd,
			const EquationOfState& eos,
			const double time,
			const double dt,
			TracerStickerNames const& tracerstickernames) const;

private:
	const vector<pair<const ConditionActionSequence::Condition*, const ConditionActionSequence::Action*> > sequence_;
	const vector<pair<const ConditionActionSequence::Condition*, const Action2*> > sequence2_;
	const SpatialReconstruction & interp_;
	mutable vector<pair<ComputationalCell, ComputationalCell> > edge_values_;
};

//! \brief Calculates flux between two regular bulk cells
class RegularFlux2 : public ConditionActionSequence2::Action2
{
public:

	/*! \brief Class constructor
	\param rs Riemann solver
	*/
	explicit RegularFlux2(const RiemannSolver& rs);

	void operator()
		(const Edge& edge,
			const size_t index,
			const Tessellation& tess,
			const Vector2D& edge_velocity,
			const vector<ComputationalCell>& cells,
			const EquationOfState& eos,
			const bool aux,
			const pair<ComputationalCell, ComputationalCell> & edge_values,
			Extensive &res, double time,
			TracerStickerNames const& tracerstickernames) const;

private:

	const RiemannSolver& rs_;
};


//! \brief Calculates flux assuming rigid wall boundary conditions
class RigidWallFlux2 : public ConditionActionSequence2::Action2
{
public:

	/*! \brief Class constructor
	\param rs Riemann solver
	*/
	explicit RigidWallFlux2(const RiemannSolver& rs);

	void operator()
		(const Edge& edge,
			const size_t index,
			const Tessellation& tess,
			const Vector2D& edge_velocity,
			const vector<ComputationalCell>& cells,
			const EquationOfState& eos,
			const bool aux,
			const pair<ComputationalCell, ComputationalCell> & edge_values,
			Extensive &res, double time,
			TracerStickerNames const& tracerstickernames) const;

private:
	const RiemannSolver& rs_;
};

//! \brief Allows matter to flow in only one direction
class Ratchet : public ConditionActionSequence2::Action2
{
public:

	/*! \brief Class constructor
	  \param rs Riemann solver
	\param in If the ratchet allows inflow or outflow
	*/
	Ratchet(const RiemannSolver& rs, const bool in);

	void operator()
		(const Edge& edge,
			const size_t index,
			const Tessellation& tess,
			const Vector2D& edge_velocity,
			const vector<ComputationalCell>& cells,
			const EquationOfState& eos,
			const bool aux,
			const pair<ComputationalCell, ComputationalCell> & edge_values,
			Extensive &res, double time,
			TracerStickerNames const& tracerstickernames) const;

private:
	const bool in_;
	const RigidWallFlux2 wall_;
	const FreeFlowFlux free_;
};

//! \brief A flux scheme that minimises mass transfer between cells
class LagrangianFlux : public ConditionActionSequence2::Action2
{
public:

  //! \brief Condition on when to apply mass transfer fix
	class LagrangianCriteria
	{
	public:
		/*! \brief Criteria for calculating mass flux or not
		\param edge Interface between cells
		\param index The index of the edge
		\param tess Tessellation
		\param cells Computational cells
		\param eos Equation of state
		\param aux Auxiliary variable for assymetric problems (true means the relevant cell is on the left side, false mean right)
		\param edge_values The interpolated values at the edge
		\param edge_velocity Velocity of the edges
		\param time The time
		\param tracerstickernames The names of the tracers and stickers
		\return True if there is no mass flux false otherwise
		*/
		virtual bool operator()(const Edge& edge,
			const size_t index,
			const Tessellation& tess,
			const Vector2D& edge_velocity,
			const vector<ComputationalCell>& cells,
			const EquationOfState& eos,
			const bool aux,
			const pair<ComputationalCell, ComputationalCell> & edge_values,
			double time,
			TracerStickerNames const& tracerstickernames) const = 0;

		virtual ~LagrangianCriteria();
	};

	/*! \brief Class constructor
	  \param rs Riemann solver with no mass flux
	  \param rs2 Riemann solver with mass flux
	\param criteria The criteria for calculating mass flux
	*/
	LagrangianFlux(const LagrangianHLLC& rs,const LagrangianHLLC& rs2,LagrangianCriteria const& criteria);

	void operator()
		(const Edge& edge,
			const size_t index,
			const Tessellation& tess,
			const Vector2D& edge_velocity,
			const vector<ComputationalCell>& cells,
			const EquationOfState& eos,
			const bool aux,
			const pair<ComputationalCell, ComputationalCell> & edge_values,
			Extensive &res, double time,
			TracerStickerNames const& tracerstickernames) const;

  /*! \brief Resets the internal variables
   */
	void Reset(void) const;

  //! \brief Velocity of the interfaces
  mutable vector<double> ws_;
  //! \brief Velocity of the edges
  mutable vector<double> edge_vel_;
  //! \brief Was this edge calculated Lagrangialy
  mutable vector<bool> Lag_calc_;
private:
  const LagrangianHLLC& rs_;
  const LagrangianHLLC& rs2_;
  const LagrangianCriteria& criteria_;
};

//! \brief Criteria for having mass flux at outer edges of domain
class WallsMassFlux : public LagrangianFlux::LagrangianCriteria
{
public:
	WallsMassFlux();

	bool operator()(const Edge& edge,
		const size_t index,
		const Tessellation& tess,
		const Vector2D& edge_velocity,
		const vector<ComputationalCell>& cells,
		const EquationOfState& eos,
		const bool aux,
		const pair<ComputationalCell, ComputationalCell> & edge_values,
		double time,
		TracerStickerNames const& tracerstickernames) const;
};

#endif // CONDITION_ACTION_SEQUENCE2_HPP
