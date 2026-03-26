%% =========================================================================
%  DIAGNOSTIC: fzero vs brentq comparison + bisection replacement
%  =========================================================================
%  Run this in MATLAB. It tests fzero on the same 5 cases that brentq
%  solved in Python, so we can see exactly where fzero diverges.
%  Then it runs a simple bisection on the same cases as a clean alternative.
%  =========================================================================

clear; clc;

%% Parameters (identical to both scripts)
rho_sus  = 1750.0;   a_sus = 1.2e-4;   n_sus = 0.35;
rho_2nd  = 1800.0;   a_2nd = 1.0e-4;   n_2nd = 0.40;
A_throat = 0.005;    c_star = 1550.0;

%% Define residual function
% Takes Pc and the grain geometry/flags as additional arguments
residual = @(Pc, Sb_s, Sb_2, sus_on, p2_on) …
(sus_on & Sb_s > 0) * rho_sus * Sb_s * a_sus * Pc^n_sus + …
(p2_on & Sb_2 > 0) * rho_2nd * Sb_2 * a_2nd * Pc^n_2nd - …
Pc * A_throat / c_star;

%% Test cases — same as Python
labels  = { ‘Sustain only, Sburn=0.30’, …
‘Both grains, Sb_s=0.30, Sb2=0.20’, …
‘Both grains, Sb_s=0.30, Sb2=0.25’, …
‘Both grains, Sb_s=0.28, Sb2=0.28’, …
‘2nd pulse only, Sburn=0.25’ };

Sb_s_vals = [0.30, 0.30, 0.30, 0.28, 0.00];
Sb_2_vals = [0.00, 0.20, 0.25, 0.28, 0.25];
sus_vals  = [true, true, true, true, false];
p2_vals   = [false, true, true, true, true];

% Expected values from Python brentq
Pc_brentq = [3990987.56, 14401288.57, 17832887.50, 19284556.52, 8083951.13];

%% Run fzero on each case
fprintf(’=====================================================================\n’);
fprintf(‘FZERO vs BRENTQ COMPARISON\n’);
fprintf(’=====================================================================\n’);

options = optimset(‘TolX’, 1.0, ‘Display’, ‘off’);

for i = 1:length(labels)
fn = @(Pc) residual(Pc, Sb_s_vals(i), Sb_2_vals(i), sus_vals(i), p2_vals(i));

```
% Check bracket endpoints
f_lo = fn(1e3);
f_hi = fn(50e6);

% Run fzero with bracket
Pc_fz = fzero(fn, [1e3, 50e6], options);

% Also try fzero with an initial guess near the expected answer
Pc_fz_guess = fzero(fn, Pc_brentq(i), options);

err_bracket = Pc_fz - Pc_brentq(i);
err_guess   = Pc_fz_guess - Pc_brentq(i);

fprintf('\n  Case %d: %s\n', i, labels{i});
fprintf('    f(1e3)   = %+.6e\n', f_lo);
fprintf('    f(50e6)  = %+.6e\n', f_hi);
fprintf('    brentq   = %.2f Pa  (%.6f MPa)  <-- REFERENCE\n', Pc_brentq(i), Pc_brentq(i)/1e6);
fprintf('    fzero[bracket]  = %.2f Pa  (%.6f MPa)  err = %+.2f Pa\n', Pc_fz, Pc_fz/1e6, err_bracket);
fprintf('    fzero[guess]    = %.2f Pa  (%.6f MPa)  err = %+.2f Pa\n', Pc_fz_guess, Pc_fz_guess/1e6, err_guess);
fprintf('    residual at fzero = %.6e\n', fn(Pc_fz));
```

end

%% Now run bisection on the same cases
fprintf(’\n\n=====================================================================\n’);
fprintf(‘BISECTION vs BRENTQ COMPARISON\n’);
fprintf(’=====================================================================\n’);

for i = 1:length(labels)
fn = @(Pc) residual(Pc, Sb_s_vals(i), Sb_2_vals(i), sus_vals(i), p2_vals(i));

```
Pc_bi = bisection_solve(fn, 1e3, 50e6, 1.0, 200);

err_bi = Pc_bi - Pc_brentq(i);
fprintf('\n  Case %d: %s\n', i, labels{i});
fprintf('    brentq     = %.2f Pa  (%.6f MPa)\n', Pc_brentq(i), Pc_brentq(i)/1e6);
fprintf('    bisection  = %.2f Pa  (%.6f MPa)  err = %+.2f Pa\n', Pc_bi, Pc_bi/1e6, err_bi);
fprintf('    residual   = %.6e\n', fn(Pc_bi));
```

end

%% =========================================================================
%  BISECTION SOLVER — drop-in replacement for fzero
%  =========================================================================
%  This is dead simple and transparent. No magic, no heuristics.
%  Guaranteed to converge if f(lo) and f(hi) have opposite signs.
%  ~50 iterations gets you to < 1 Pa on a [1e3, 50e6] bracket.
%  =========================================================================

function Pc = bisection_solve(fn, lo, hi, tol, max_iter)
% BISECTION_SOLVE  Find root of fn in [lo, hi] by bisection.
%
%   fn       - function handle, f(Pc)
%   lo, hi   - bracket endpoints (f(lo) and f(hi) must have opposite sign)
%   tol      - absolute tolerance on Pc [Pa]
%   max_iter - maximum iterations (50 is usually plenty)

```
f_lo = fn(lo);
f_hi = fn(hi);

if sign(f_lo) == sign(f_hi)
    warning('Bracket does not straddle zero! f(lo)=%.4e, f(hi)=%.4e', f_lo, f_hi);
    Pc = 0.0;
    return;
end

for iter = 1:max_iter
    mid = (lo + hi) / 2.0;
    f_mid = fn(mid);

    if abs(hi - lo) < tol
        Pc = mid;
        return;
    end

    if sign(f_mid) == sign(f_lo)
        lo = mid;
        f_lo = f_mid;
    else
        hi = mid;
        f_hi = f_mid;
    end
end

Pc = (lo + hi) / 2.0;
```

end