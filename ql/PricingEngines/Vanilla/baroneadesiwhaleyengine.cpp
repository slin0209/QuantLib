
/*
 Copyright (C) 2003 Ferdinando Ametrano

 This file is part of QuantLib, a free-software/open-source library
 for financial quantitative analysts and developers - http://quantlib.org/

 QuantLib is free software: you can redistribute it and/or modify it under the
 terms of the QuantLib license.  You should have received a copy of the
 license along with this program; if not, please email quantlib-dev@lists.sf.net
 The license is also available online at http://quantlib.org/html/license.html

 This program is distributed in the hope that it will be useful, but WITHOUT
 ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 FOR A PARTICULAR PURPOSE.  See the license for more details.
*/

/*! \file baroneadesiwhaleyengine.cpp
    \brief American option engine using Barone-Adesi and Whaley approximation
*/

#include <ql/PricingEngines/Vanilla/vanillaengines.hpp>
#include <ql/PricingEngines/blackformula.hpp>

namespace QuantLib {


    namespace {

        // critical commodity price
        double Kc(const Handle<StrikedTypePayoff>& payoff,
            double T,
            double riskFreeDiscount, double dividendDiscount,
            double variance) {

            // Calculation of seed value, Si
            double n= 2.0*QL_LOG(dividendDiscount/riskFreeDiscount)/(variance);
            double m=-2.0*QL_LOG(riskFreeDiscount)/(variance);
            double bT = QL_LOG(dividendDiscount/riskFreeDiscount);

            double qu, Su, h, Si;
            switch (payoff->optionType()) {
                case Option::Call:
                    qu = (-(n-1.0) + QL_SQRT(((n-1.0)*(n-1.0)) + 4.0*m))/2.0;
                    Su = payoff->strike() / (1.0 - 1.0/qu);
                    h = -(bT + 2.0*QL_SQRT(variance)) * payoff->strike() / (Su - payoff->strike());
                    Si = payoff->strike() + (Su - payoff->strike()) * (1.0 - QL_EXP(h));
                case Option::Put:
                    qu = (-(n-1.0) - QL_SQRT(((n-1.0)*(n-1.0)) + 4.0*m))/2.0;
                    Su = payoff->strike() / (1.0 - 1.0/qu);
                    h = (bT - 2.0*QL_SQRT(variance)) * payoff->strike() / (payoff->strike() - Su);
                    Si = Su + (payoff->strike() - Su) * QL_EXP(h);
                default:
                    throw Error("BaroneAdesiWhaleyApproximationEngine::"
                        "calculate() :"
                        "unknown option type");
            }

            double K = -2.0*QL_LOG(riskFreeDiscount)/
                (variance*(1.0-riskFreeDiscount));

            double forwardSi = Si * dividendDiscount / riskFreeDiscount;
            double d1 = (QL_LOG(forwardSi/payoff->strike()) + 0.5*variance)/QL_SQRT(variance);

            CumulativeNormalDistribution CND;
            BlackFormula black(Si, forwardSi, riskFreeDiscount, variance,
                payoff);

            double Q, LHS, RHS, bi;
            switch (payoff->optionType()) {
                case Option::Call:
                    Q = (-(n-1.0) + QL_SQRT(((n-1.0)*(n-1.0)) + 4 * K)) / 2;
                    LHS = Si - payoff->strike();
                    RHS = black.value() + (1 - dividendDiscount * CND(d1)) * Si / Q;
                    bi =  dividendDiscount * CND(d1) * (1 - 1/Q) + (1 - dividendDiscount * CND(d1) / QL_SQRT(variance)) / Q;
                    // Newton Raphson algorithm for finding critical price Si
                    while (QL_FABS(LHS - RHS)/payoff->strike() > 1e-5) {
                        Si = (payoff->strike() + RHS - bi * Si) / (1 - bi);
                        forwardSi = Si * dividendDiscount / riskFreeDiscount;
                        black = BlackFormula(Si, forwardSi, riskFreeDiscount, variance, payoff);
                        d1 = (QL_LOG(forwardSi/payoff->strike()) + 0.5*variance)/QL_SQRT(variance);
                        LHS = Si - payoff->strike();
                        RHS = black.value() + (1 - dividendDiscount * CND(d1)) * Si / Q;
                        bi = dividendDiscount * CND(d1) * (1 - 1 / Q) + (1 - dividendDiscount * CND.derivative(d1) / QL_SQRT(variance)) / Q;
                    }
                case Option::Put:
                    Q = (-(n-1.0) - QL_SQRT(((n-1.0)*(n-1.0)) + 4 * K)) / 2;
                    LHS = payoff->strike() - Si;
                    RHS = black.value() - (1 - dividendDiscount * CND(-d1)) * Si / Q;
                    bi = -dividendDiscount * CND(-d1) * (1 - 1/Q) - (1 + dividendDiscount * CND.derivative(-d1) / QL_SQRT(variance)) / Q;
                    // Newton Raphson algorithm for finding critical price Si
                    while (QL_FABS(LHS - RHS)/payoff->strike() > 1e-5) {
                        Si = (payoff->strike() - RHS + bi * Si) / (1 + bi);
                        forwardSi = Si * dividendDiscount / riskFreeDiscount;
                        black = BlackFormula(Si, forwardSi, riskFreeDiscount, variance, payoff);
                        d1 = (QL_LOG(forwardSi/payoff->strike()) + 0.5*variance)/QL_SQRT(variance);
                        LHS = payoff->strike() - Si;
                        RHS = black.value() - (1 - dividendDiscount * CND(-d1)) * Si / Q;
                        bi = -dividendDiscount * CND(-d1) * (1 - 1 / Q) - (1 + dividendDiscount * CND(-d1) / QL_SQRT(variance)) / Q;
                    }
                default:
                    throw Error("BaroneAdesiWhaleyApproximationEngine::"
                        "calculate() :"
                        "unknown option type");
            }

            return Si;
        }
    }

    void BaroneAdesiWhaleyApproximationEngine::calculate() const {

        QL_REQUIRE(arguments_.exercise->type() == Exercise::American,
                   "BaroneAdesiWhaleyApproximationEngine::calculate() : "
                   "not an American Option");

        #if defined(HAVE_BOOST)
        Handle<AmericanExercise> ex = 
            boost::dynamic_pointer_cast<AmericanExercise>(arguments_.exercise);
        QL_REQUIRE(ex,
                   "BaroneAdesiWhaleyApproximationEngine: non-American exercise given");
        #else
        Handle<AmericanExercise> ex = arguments_.exercise;
        #endif
        QL_REQUIRE(!ex->payoffAtExpiry(),
                   "BaroneAdesiWhaleyApproximationEngine::calculate() : "
                   "payoff at expiry not handled");

        #if defined(HAVE_BOOST)
        Handle<StrikedTypePayoff> payoff =
            boost::dynamic_pointer_cast<StrikedTypePayoff>(arguments_.payoff);
        QL_REQUIRE(payoff,
                   "AnalyticEuropeanEngine: non-striked payoff given");
        #else
        Handle<StrikedTypePayoff> payoff = arguments_.payoff;
        #endif

        double variance = arguments_.volTS->blackVariance(
            ex->lastDate(), payoff->strike());
        DiscountFactor dividendDiscount =
            arguments_.dividendTS->discount(ex->lastDate());
        DiscountFactor riskFreeDiscount =
            arguments_.riskFreeTS->discount(ex->lastDate());
        double forwardPrice = arguments_.underlying *
            dividendDiscount / riskFreeDiscount;

        CumulativeNormalDistribution CND;
        BlackFormula black(arguments_.underlying,
            forwardPrice, riskFreeDiscount,
            variance, payoff);

        switch (payoff->optionType()) {
            case Option::Call:
                results_.value = 0.0;
                break;
            case Option::Put:
                results_.value = 0.0;
                break;
            case Option::Straddle:
                results_.value = 0.0;
                break;
            default:
                throw Error("BaroneAdesiWhaleyApproximationEngine::"
                    "calculate() :"
                    "unknown option type");
        }


        Time T = arguments_.riskFreeTS->dayCounter().yearFraction(
            arguments_.riskFreeTS->referenceDate(),
            arguments_.exercise->lastDate());
        Rate r = -QL_LOG(riskFreeDiscount)/T;
        Rate q = -QL_LOG(dividendDiscount)/T;
        Spread b = QL_LOG(dividendDiscount/riskFreeDiscount)/T;
        double v = QL_SQRT(variance/T);
        double Sk = Kc(payoff, T, r, b, v);

        double forwardSk = Sk * dividendDiscount / riskFreeDiscount;
        double d1 = (QL_LOG(forwardSk/payoff->strike()) + 0.5*variance)/QL_SQRT(variance);

        
        double n = 2.0*QL_LOG(dividendDiscount/riskFreeDiscount)/(variance);
        double K = -2.0*QL_LOG(riskFreeDiscount)/
            (variance*(1.0-riskFreeDiscount));
        double Q, a;
        switch (payoff->optionType()) {
            case Option::Call:
                if (dividendDiscount>=1.0) {
                    results_.value = black.value();
                    results_.delta = black.delta();
                    results_.deltaForward = black.deltaForward();
                    results_.elasticity = black.elasticity();
                    results_.gamma = black.gamma();

                    Time t = arguments_.riskFreeTS->dayCounter().yearFraction(
                        arguments_.riskFreeTS->referenceDate(),
                        arguments_.exercise->lastDate());
                    results_.rho = black.rho(t);

                    t = arguments_.dividendTS->dayCounter().yearFraction(
                        arguments_.dividendTS->referenceDate(),
                        arguments_.exercise->lastDate());
                    results_.dividendRho = black.dividendRho(t);

                    t = arguments_.volTS->dayCounter().yearFraction(
                        arguments_.volTS->referenceDate(),
                        arguments_.exercise->lastDate());
                    results_.vega = black.vega(t);
                    results_.theta = black.theta(t);
                    results_.thetaPerDay = black.thetaPerDay(t);

                    results_.strikeSensitivity = black.strikeSensitivity();
                    results_.itmProbability = black.itmProbability();
                } else {
                    Q = (-(n-1.0) + QL_SQRT(((n-1.0)*(n-1.0))+4.0*K))/2.0;
                    a =  (Sk/Q) * (1.0 - dividendDiscount * CND(d1));
                    if (arguments_.underlying<Sk) {
                        results_.value = black.value() + a * QL_POW((arguments_.underlying/Sk), Q);
                    } else {
                        results_.value = arguments_.underlying - payoff->strike();
                    }
                }
                break;
            case Option::Put:
                Q = (-(n-1.0) - QL_SQRT(((n-1.0)*(n-1.0))+4.0*K))/2.0;
                a = -(Sk/Q) * (1.0 - dividendDiscount * CND(-d1));
                if (arguments_.underlying>Sk) {
                    results_.value = black.value() + a * QL_POW((arguments_.underlying/Sk), Q);
                } else {
                    results_.value = payoff->strike() - arguments_.underlying;
                }
                break;
            default:
                throw Error("BaroneAdesiWhaleyApproximationEngine::"
                    "calculate() :"
                    "unknown option type");
        }


    }

}
