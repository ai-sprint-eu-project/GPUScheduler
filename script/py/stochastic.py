# TO START THE WEB APP
# make api

from flask import Flask, request, jsonify
from waitress import serve
from werkzeug.serving import WSGIRequestHandler
import logging
import pyomo.environ as pyo
import numpy as np
import pandas as pd

app = Flask(__name__)

solution = {
    "x"     : [1],
    "obj"   : 5,
    "tc"    : "optimal",
}

@app.get("/solution")
def get_solution():
    return solution

@app.put("/solution")
def solve():
    if request.is_json:
        # Get data
        s   = (request.get_json())["s"]
        p   = (request.get_json())["p"]
        d   = (request.get_json())["d"]
        e   = (request.get_json()).get("e")
        e   = 0 if e == None else e
        me  = (request.get_json())["max_e"]
        dis = (request.get_json())["distribution"]

        index_dis = np.where(distributions == dis)[0][0]
        w = W[index_dis]
        f = F[index_dis]
        w = w * me / 100
        it = np.searchsorted(w, e)
        w = w[it:]
        if w[0] != e:
            w = np.insert(w, 0, e)
            it-=1
        w = w.tolist()
        f = f[it:] / f[it]
        f = f.tolist()

        hf = 0.01
        r  = 1

        N = len(s)
        M = len(f)

        # Solve
        tmin = (me - e) / s[0]
        tmax = (me - e) / s[-1]

        if tmin <= d or M == 1:
            x = [me] * N
            x.insert(0, e)
            solution["x"] = x
            solution["obj"] = 0
            solution["tc"] = "loose"
        elif tmax > d:
            x = [e] * N
            x.append(me)
            solution["x"] = x
            solution["obj"] = 1e6
            solution["tc"] = "tardiness"
        else:
            data = {None:
                {
                'N'     : {None:N},
                's'     : {i+1:s[i] for i in range(N)},
                'p'     : {i+1:p[i] for i in range(N)},
                'd'     : {None:d},
                'M'     : {None:M},
                'w'     : {i:w[i] for i in range(M+1)},
                'f'     : {i+1:f[i] for i in range(M)},
                'hf'    : {None:hf},
                'r'     : {None:r}
                }
            }
            instance = m.create_instance(data)
            solver   = pyo.SolverFactory('ipopt')
            solver.options['max_iter'] = 10
            results  = solver.solve(instance)
            # Get x
            x = []
            for v in instance.component_data_objects(pyo.Var, active=True):
                x.append(pyo.value(v))
            # Update solution
            solution["x"]   = x
            solution["obj"] = pyo.value(instance.obj)
            solution["tc"]  = str(results.solver.termination_condition)

        return solution;

    return {"error": "Request must be JSON"}, 415
  
if __name__ == "__main__":
    # load distributions
    df = pd.read_csv("build/data/distributions.csv")
    distributions = df.columns.values
    W = []
    F = []
    for dis in distributions:
        df_loc = df[df[dis]!=0]
        p = df_loc[dis].values
        f = 1 - np.cumsum(p / np.sum(p))
        f = np.insert(f, 0, 1)
        w = df_loc[dis].index.values + 1
        w = np.insert(w, 0, 0)
        if (w[-1] != 100):
            w = np.append(w, 100)
            f[-1] = 0
        else:
            f = f[0:-1]
        F.append(f)
        W.append(w)

    ## Define model
    m = pyo.AbstractModel()

    # gpus parameter
    m.N  = pyo.Param()
    m.I  = pyo.RangeSet(1, m.N)
    m.I0 = pyo.RangeSet(0, m.N)
    m.s  = pyo.Param(m.I)
    m.p  = pyo.Param(m.I)

    # job paramater
    m.d  = pyo.Param()
    m.M  = pyo.Param()
    m.J  = pyo.RangeSet(1, m.M)
    m.J0 = pyo.RangeSet(0, m.M)
    m.w  = pyo.Param(m.J0)
    m.f  = pyo.Param(m.J)

    # approx paramater
    m.r  = pyo.Param()
    m.hf = pyo.Param()
    def define_h(m, j):
        return (m.w[j] - m.w[j-1]) * m.hf
    m.h  = pyo.Expression(m.J, rule=define_h)
    def define_wp(m, j):
        return m.w[j] - m.h[j]
    m.wp = pyo.Expression(m.J, rule=define_wp)

    # integral
    def simple_integral(m, j):
        df = m.f[j] if j == m.M else m.f[j] - m.f[j+1]
        return (m.w[j] - m.w[j-1]) * m.f[j] - df * m.h[j] / (m.r + 1)
    m.Fs = pyo.Expression(m.J, rule=simple_integral)

    # variable
    m.x = pyo.Var(m.I0, within=pyo.NonNegativeReals)

    # objective Function
    def approx_integral(m, j, b):
        df = m.f[j] if j == m.M else m.f[j] - m.f[j+1]
        frac = b / m.h[j]
        return df / (m.r + 1) * frac**m.r * b

    def local_integral(m, j, b):
        a = m.w[j-1]
        return (b - a) * m.f[j] - approx_integral(m, j, b-m.wp[j]) * (b > m.wp[j])

    def global_integral(m, i):
        integral = 0
        for j in range(1, m.M + 1):
            integral += (m.w[j] <= m.x[i]) * m.Fs[j] \
                        + (m.w[j-1] < m.x[i]) * (m.x[i] < m.w[j]) * local_integral(m, j, m.x[i])
        return integral
    m.F = pyo.Expression(m.I0, rule=global_integral)

    def obj_rule(m):
        obj = m.p[m.N] * m.F[m.N]
        for i in range(2, m.N + 1):
            obj -= (m.p[i] - m.p[i-1]) * m.F[i-1]
        return obj
    m.obj = pyo.Objective(rule=obj_rule)

    # constraints
    def first(m):
        return m.x[0] == m.w[0]
    m.cons_first = pyo.Constraint(rule=first)

    def last(m):
        return m.x[m.N] == m.w[m.M]
    m.cons_last = pyo.Constraint(rule=last)

    def increasing(m, i):
        return m.x[i-1] <= m.x[i]
    m.cons_increasing = pyo.Constraint(m.I, rule=increasing)

    def deadline(m):
        return sum((m.x[i]-m.x[i-1]) / m.s[i] for i in m.I) == m.d
    m.cons_deadline = pyo.Constraint(rule=deadline)

    # Suppress warnings
    logging.getLogger('pyomo.core').setLevel(logging.ERROR)

    # Start api
    WSGIRequestHandler.protocol_version = "HTTP/1.1"
    serve(app, port=5000)
