%==========================================================================
% srm_dual_pulse_overlap.m
%
% Dual-Pulse Solid Rocket Motor Overlap Burn Model
%
% Solves for coupled chamber pressure, mass flow rate, and timing when
% two grains (sustain + pulse 2) burn simultaneously in a shared chamber.
%
% Data Structure:
%   Each grain is a struct with its OWN independent %burn grid.
%   Required fields per grain:
%     .pct       - [N x 1] fractional burn (0 to 1), grain's native grid
%     .Pc_iso    - [N x 1] isolated equilibrium chamber pressure [Pa]
%     .mdot_iso  - [N x 1] isolated mass flow rate [kg/s]
%     .rdot_iso  - [N x 1] isolated burn rate [m/s]
%     .Sburn     - [N x 1] burn surface area at each %burn [m^2]
%     .rho       - scalar, propellant density [kg/m^3]
%     .web       - scalar, total web thickness [m]
%     .name      - string, grain identifier
%
%   Burn rate specification (ONE of the following):
%     Option A (scalar, constant a and n):
%       .a_br    - scalar burn rate coefficient
%       .n_br    - scalar burn rate exponent
%     Option B (pressure-dependent, piecewise a and n):
%       .br_table - [M x 3] table: [Pc_lower_bound, a, n]
%                   Rows sorted ascending by Pc. At a given Pc, the row
%                   with the largest lower bound <= Pc is used.
%                   Example for 3 pressure regimes:
%                     [  0,    3.20e-5, 0.35;
%                       5e6,  2.80e-5, 0.38;
%                      10e6,  2.50e-5, 0.40 ]
%                   Example for single a,n (equivalent to Option A):
%                     [0, 2.50e-5, 0.38]
%
%   Throat area specification (optional):
%     .At       - [N x 1] nozzle throat area at each %burn point [m^2]
%                 If omitted, the motor-level scalar At is used.
%
%   Derived fields (computed by loader or by user):
%     .dt        - [N-1 x 1] time increments between %burn steps [s]
%     .t_cum     - [N x 1] cumulative time from 0 [s]
%
% Usage:
%   1. Edit load_grain_data() OR replace it with your own loader
%   2. Set t_ignite_p2 and solver parameters in USER CONFIGURATION
%   3. Run: >> srm_dual_pulse_overlap
%==========================================================================

clear; clc; close all;

%% ==================== USER CONFIGURATION ==============================
t_ignite_p2 = 2.0;   % [s] When Pulse 2 ignites (after motor start)

% Iteration control
max_iter = 50;
tol      = 1e-5;     % Relative Pc convergence tolerance

%% ==================== COMMON MOTOR PARAMETERS =========================
At_const = 1.0e-3;   % [m^2] Default throat area (used if grain has no .At)
Cstar    = 1550;      % [m/s] Characteristic velocity

%% ==================== LOAD GRAIN DATA =================================
[sus, p2] = load_grain_data(At_const, Cstar);

% --- Normalize burn rate data to br_table format -----------------------
% If a grain uses scalar a_br/n_br, convert to a single-row br_table
% so the solver only deals with one format.
sus = normalize_grain(sus, At_const);
p2  = normalize_grain(p2,  At_const);

fprintf('--- Grain Data Loaded ---\n');
fprintf('  Sustain: %d points, %d burn rate regime(s), At %s, burn time = %.2f s\n', ...
    length(sus.pct), size(sus.br_table,1), at_label(sus), sus.t_cum(end));
fprintf('  Pulse 2: %d points, %d burn rate regime(s), At %s, burn time = %.2f s\n', ...
    length(p2.pct), size(p2.br_table,1), at_label(p2), p2.t_cum(end));

%% ==================== DETERMINE OVERLAP PARAMETERS ====================
if t_ignite_p2 <= 0
    error('Ignition time must be positive.');
end
if t_ignite_p2 >= sus.t_cum(end)
    error('Pulse 2 ignition (%.2f s) is after sustain burnout (%.2f s). No overlap.', ...
        t_ignite_p2, sus.t_cum(end));
end

pct_sus_at_ign = interp1(sus.t_cum, sus.pct, t_ignite_p2, 'linear');

sus_time_remaining = sus.t_cum(end) - t_ignite_p2;
p2_time_remaining  = p2.t_cum(end);

fprintf('\n--- Overlap Configuration ---\n');
fprintf('  Pulse 2 ignition time:       %.2f s\n', t_ignite_p2);
fprintf('  Sustain %%burn at ignition:   %.1f%%\n', pct_sus_at_ign * 100);
fprintf('  Sustain remaining burn time:  %.2f s\n', sus_time_remaining);
fprintf('  Pulse 2 total burn time:      %.2f s\n', p2_time_remaining);

%% ==================== BUILD WORKING DATA FOR OVERLAP PHASE ============
% Extract sustain's remaining data
sus_rem_mask = sus.pct >= pct_sus_at_ign;
sus_rem_pct  = sus.pct(sus_rem_mask);

if sus_rem_pct(1) > pct_sus_at_ign + 1e-12
    sus_rem_pct = [pct_sus_at_ign; sus_rem_pct];
end

sus_rem.Pc_iso   = interp1(sus.pct, sus.Pc_iso,   sus_rem_pct, 'linear');
sus_rem.mdot_iso = interp1(sus.pct, sus.mdot_iso,  sus_rem_pct, 'linear');
sus_rem.rdot_iso = interp1(sus.pct, sus.rdot_iso,  sus_rem_pct, 'linear');
sus_rem.Sburn    = interp1(sus.pct, sus.Sburn,     sus_rem_pct, 'linear');
sus_rem.At       = interp1(sus.pct, sus.At,         sus_rem_pct, 'linear');
sus_rem.br_table = sus.br_table;
sus_rem.rho      = sus.rho;
sus_rem.web      = sus.web;
sus_rem.name     = sus.name;

dw_sus = sus.web * diff(sus_rem_pct);
rdot_avg_sus = 0.5 * (sus_rem.rdot_iso(1:end-1) + sus_rem.rdot_iso(2:end));
sus_rem.dt_iso = dw_sus ./ rdot_avg_sus;
sus_rem.t_cum  = [0; cumsum(sus_rem.dt_iso)];

% Pulse 2: full native grid
p2_rem_pct = p2.pct;
p2_rem.Pc_iso   = p2.Pc_iso;
p2_rem.mdot_iso = p2.mdot_iso;
p2_rem.rdot_iso = p2.rdot_iso;
p2_rem.Sburn    = p2.Sburn;
p2_rem.At       = p2.At;
p2_rem.br_table = p2.br_table;
p2_rem.rho      = p2.rho;
p2_rem.web      = p2.web;
p2_rem.name     = p2.name;
p2_rem.dt_iso   = p2.dt;
p2_rem.t_cum    = p2.t_cum;

% Select primary grain (longer remaining burn = time reference)
if p2_time_remaining >= sus_time_remaining
    prim      = p2_rem;   prim_pct = p2_rem_pct;   prim_name = p2.name;
    sec       = sus_rem;  sec_pct  = sus_rem_pct;   sec_name  = sus.name;
else
    prim      = sus_rem;  prim_pct = sus_rem_pct;   prim_name = sus.name;
    sec       = p2_rem;   sec_pct  = p2_rem_pct;    sec_name  = p2.name;
end
fprintf('  Primary time reference:       %s (%d points)\n', prim_name, length(prim_pct));
fprintf('  Secondary grain:              %s (%d points)\n\n', sec_name, length(sec_pct));

%% ==================== ITERATIVE OVERLAP SOLVER ========================
fprintf('--- Iterative Solver ---\n');

N_prim = length(prim_pct);

prim_dt  = prim.dt_iso;
prim_t   = prim.t_cum;
sec_dt   = sec.dt_iso;
sec_t    = sec.t_cum;

Pc_combined    = zeros(N_prim, 1);
Pc_prev        = zeros(N_prim, 1);
mdot_prim_corr = zeros(N_prim, 1);
mdot_sec_corr  = zeros(N_prim, 1);

Pc_history = {};
t_history  = {};

converged = false;

for iter = 1:max_iter

    % ----- Step 1: Map secondary %burn onto primary's time grid --------
    sec_pct_on_prim = zeros(N_prim, 1);
    sec_active      = true(N_prim, 1);

    for j = 1:N_prim
        tj = prim_t(j);
        if tj <= sec_t(end)
            sec_pct_on_prim(j) = interp1(sec_t, sec_pct, tj, 'linear');
        else
            sec_pct_on_prim(j) = sec_pct(end);
            sec_active(j) = false;
        end
    end

    % Look up secondary's isolated Sburn at mapped %burn values
    sec_Sburn_mapped = interp1(sec_pct, sec.Sburn, sec_pct_on_prim, 'linear', 'extrap');
    sec_Sburn_mapped(~sec_active) = 0;

    % Look up secondary's isolated Pc/mdot for initial guess and history
    sec_Pc_mapped   = interp1(sec_pct, sec.Pc_iso,   sec_pct_on_prim, 'linear', 'extrap');
    sec_mdot_mapped = interp1(sec_pct, sec.mdot_iso,  sec_pct_on_prim, 'linear', 'extrap');
    sec_mdot_mapped(~sec_active) = 0;
    sec_Pc_mapped(~sec_active)   = 0;

    % ----- Step 2: Solve Pc at each %burn point ------------------------
    % Uses the FUNDAMENTAL mass balance, not the ratio shortcut.
    % This correctly handles pressure-dependent a,n and variable At.
    %   Sum_k[ Sburn_k * rho_k * rdot_k(Pc) ] = Pc * At(pct) / Cstar
    % where rdot_k(Pc) = a_k(Pc) * Pc^(n_k(Pc)) from the burn rate table.

    opts = optimset('TolX', 1e-8, 'Display', 'off');

    for j = 1:N_prim
        % Primary grain quantities at this %burn
        Sb_p  = prim.Sburn(j);
        rho_p = prim.rho;
        At_j  = prim.At(j);   % Throat area from primary grain's profile

        if sec_active(j)
            Sb_s  = sec_Sburn_mapped(j);
            rho_s = sec.rho;

            f = @(Pc) Sb_p * rho_p * eval_rdot(Pc, prim.br_table) ...
                    + Sb_s * rho_s * eval_rdot(Pc, sec.br_table) ...
                    - Pc * At_j / Cstar;

            if iter == 1
                Pc0 = prim.Pc_iso(j) + sec_Pc_mapped(j);
            else
                Pc0 = Pc_prev(j);
            end
        else
            f = @(Pc) Sb_p * rho_p * eval_rdot(Pc, prim.br_table) ...
                    - Pc * At_j / Cstar;
            Pc0 = prim.Pc_iso(j);
        end

        Pc_lo = 1e3;
        Pc_hi = max(Pc0 * 5, 100e6);
        Pc_combined(j) = fzero(f, [Pc_lo, Pc_hi], opts);

        % Store corrected mdot for each grain
        mdot_prim_corr(j) = Sb_p * rho_p * eval_rdot(Pc_combined(j), prim.br_table);
        if sec_active(j)
            mdot_sec_corr(j) = Sb_s * rho_s * eval_rdot(Pc_combined(j), sec.br_table);
        else
            mdot_sec_corr(j) = 0;
        end
    end

    % ----- Store iteration history --------------------------------------
    Pc_history{iter} = Pc_combined;
    t_history{iter}  = prim_t + t_ignite_p2;

    % ----- Step 3: Check convergence -----------------------------------
    if iter > 1
        valid = Pc_prev > 0;
        rel_change = max(abs(Pc_combined(valid) - Pc_prev(valid)) ./ Pc_prev(valid));
        fprintf('  Iter %2d: max |dPc/Pc| = %.2e', iter, rel_change);
        if rel_change < tol
            fprintf('  ** CONVERGED **\n');
            converged = true;
            break;
        end
        fprintf('\n');
    else
        fprintf('  Iter %2d: initial solve complete\n', iter);
    end
    Pc_prev = Pc_combined;

    % ----- Step 4: Correct time vectors --------------------------------
    % dt scales as rdot_old / rdot_new (fundamental form, handles
    % pressure-dependent a,n correctly).

    % Primary grain
    for j = 1:length(prim_dt)
        Pc_old_avg = 0.5 * (prim.Pc_iso(j) + prim.Pc_iso(j+1));
        Pc_new_avg = 0.5 * (Pc_combined(j) + Pc_combined(j+1));
        if isfinite(Pc_new_avg) && Pc_new_avg > 0
            rdot_old = eval_rdot(Pc_old_avg, prim.br_table);
            rdot_new = eval_rdot(Pc_new_avg, prim.br_table);
            prim_dt(j) = prim.dt_iso(j) * rdot_old / rdot_new;
        end
    end
    prim_t = [0; cumsum(prim_dt)];

    % Secondary grain: map Pc to secondary's %burn grid (in pct-space)
    active_idx = find(sec_active);
    if ~isempty(active_idx)
        sec_pct_mapped = sec_pct_on_prim(active_idx);
        Pc_mapped      = Pc_combined(active_idx);

        [sec_pct_mapped, uniq_idx] = unique(sec_pct_mapped);
        Pc_mapped = Pc_mapped(uniq_idx);

        if length(sec_pct_mapped) >= 2
            Pc_at_sec_pct = interp1(sec_pct_mapped, Pc_mapped, sec_pct, ...
                                    'linear', 'extrap');
            Pc_at_sec_pct = max(Pc_at_sec_pct, 1e3);

            for j = 1:length(sec_dt)
                Pc_old_avg = 0.5 * (sec.Pc_iso(j) + sec.Pc_iso(j+1));
                Pc_new_avg = 0.5 * (Pc_at_sec_pct(j) + Pc_at_sec_pct(j+1));
                rdot_old = eval_rdot(Pc_old_avg, sec.br_table);
                rdot_new = eval_rdot(Pc_new_avg, sec.br_table);
                sec_dt(j) = sec.dt_iso(j) * rdot_old / rdot_new;
            end
        end
    end
    sec_t = [0; cumsum(sec_dt)];

end  % iteration loop

if ~converged
    warning('Solver did not converge within %d iterations (last dPc = %.2e)', ...
        max_iter, rel_change);
end

%% ==================== RECORD OVERLAP RESULTS ==========================
overlap_t_local  = prim_t;
overlap_t_global = prim_t + t_ignite_p2;

sec_burnout_local  = sec_t(end);
sec_burnout_global = sec_burnout_local + t_ignite_p2;

fprintf('\n--- Results ---\n');
fprintf('  %s burnout (global): %.2f s\n', sec_name, sec_burnout_global);
fprintf('  %s burnout (global): %.2f s\n', prim_name, overlap_t_global(end));
fprintf('  Overlap duration:           %.2f s\n', sec_burnout_local);
fprintf('  Peak Pc during overlap:     %.2f MPa\n', max(Pc_combined)/1e6);

%% ==================== ASSEMBLE FULL TIMELINE ==========================
% Phase 1: Sustain burning alone
phase1_mask = sus.t_cum < t_ignite_p2;
phase1_t    = sus.t_cum(phase1_mask);
phase1_Pc   = sus.Pc_iso(phase1_mask);
phase1_At   = sus.At(phase1_mask);

% Phases 2+3: Overlap + trailing grain
phase23_t  = overlap_t_global;
phase23_Pc = Pc_combined;
phase23_At = prim.At;   % Throat area from primary grain's profile

% Combine
full_t  = [phase1_t;  phase23_t(2:end)];
full_Pc = [phase1_Pc; phase23_Pc(2:end)];
full_At = [phase1_At; phase23_At(2:end)];

% Total mdot from steady state (using variable At)
full_mdot = full_Pc .* full_At / Cstar;

%% ==================== PLOTTING ========================================
fig = figure('Position', [50, 50, 1400, 900], 'Color', 'w');
sgtitle('Dual-Pulse SRM Overlap Burn Model', 'FontSize', 15, 'FontWeight', 'bold');

% ---- Plot 1: Full Chamber Pressure Timeline ---
subplot(2, 2, 1);
plot(full_t, full_Pc / 1e6, 'b-', 'LineWidth', 2.0); hold on;
xline(t_ignite_p2, 'r--', 'LineWidth', 1.5);
xline(sec_burnout_global, 'm--', 'LineWidth', 1.5);
xlabel('Time [s]'); ylabel('P_c [MPa]');
title('Combined Chamber Pressure (Full Timeline)');
legend('P_c (combined)', 'Pulse 2 Ignition', ...
    sprintf('%s Burnout', sec_name), 'Location', 'best');
grid on; set(gca, 'FontSize', 11);

% ---- Plot 2: Isolated vs Combined Pc ---
subplot(2, 2, 2);
plot(sus.t_cum, sus.Pc_iso / 1e6, 'b--', 'LineWidth', 1.5); hold on;
plot(p2.t_cum + t_ignite_p2, p2.Pc_iso / 1e6, 'r--', 'LineWidth', 1.5);
plot(full_t, full_Pc / 1e6, 'k-', 'LineWidth', 2.0);
xline(t_ignite_p2, 'g--', 'LineWidth', 1.0);
xlabel('Time [s]'); ylabel('P_c [MPa]');
title('Isolated Grain Pressures vs Combined');
legend('Sustain (isolated)', 'Pulse 2 (isolated)', 'Combined', ...
    'Pulse 2 Ignition', 'Location', 'best');
grid on; set(gca, 'FontSize', 11);

% ---- Plot 3: Mass Flow Rates During Overlap ---
subplot(2, 2, 3);
mdot_total_overlap = mdot_prim_corr + mdot_sec_corr;
plot(overlap_t_global, mdot_total_overlap, 'k-', 'LineWidth', 2.0); hold on;
plot(overlap_t_global, mdot_prim_corr, 'r-', 'LineWidth', 1.5);
plot(overlap_t_global, mdot_sec_corr,  'b-', 'LineWidth', 1.5);
xline(sec_burnout_global, 'm--', 'LineWidth', 1.0);
xlabel('Time [s]'); ylabel('Mass Flow Rate [kg/s]');
title('Mass Flow Rates During Overlap');
legend('\Sigma mdot', prim_name, sec_name, ...
    sprintf('%s Burnout', sec_name), 'Location', 'best');
grid on; set(gca, 'FontSize', 11);

% ---- Plot 4: Iteration Convergence History (Full Timeline) ---
subplot(2, 2, 4);
n_iters = length(Pc_history);
hold on;
for k = 1:n_iters - 1
    iter_full_t  = [phase1_t;  t_history{k}(2:end)];
    iter_full_Pc = [phase1_Pc; Pc_history{k}(2:end)];
    plot(iter_full_t, iter_full_Pc / 1e6, '-', ...
        'Color', [0.6 0.6 0.6 0.35], 'LineWidth', 1.0);
end
iter_full_t  = [phase1_t;  t_history{n_iters}(2:end)];
iter_full_Pc = [phase1_Pc; Pc_history{n_iters}(2:end)];
plot(iter_full_t, iter_full_Pc / 1e6, 'k-', 'LineWidth', 2.5);
xline(t_ignite_p2, 'g--', 'LineWidth', 1.0);
xline(sec_burnout_global, 'm--', 'LineWidth', 1.0);
xlabel('Time [s]'); ylabel('P_c [MPa]');
title('P_c Convergence Across Iterations');
legend(sprintf('Iter 1-%d', n_iters - 1), sprintf('Iter %d (converged)', n_iters), ...
    'Pulse 2 Ignition', sprintf('%s Burnout', sec_name), 'Location', 'best');
grid on; set(gca, 'FontSize', 11);

set(fig, 'PaperPositionMode', 'auto');
fprintf('\n--- Simulation Complete ---\n');


%% ======================================================================
%  HELPER FUNCTIONS
%  ======================================================================

function rd = eval_rdot(Pc, br_table)
%EVAL_RDOT  Compute burn rate from pressure and piecewise burn rate table.
%
%   br_table is [M x 3]: each row = [Pc_lower_bound, a, n]
%   Rows sorted ascending by Pc_lower_bound.
%   Returns rdot = a * Pc^n using the appropriate a,n for the given Pc.

    idx = find(br_table(:,1) <= Pc, 1, 'last');
    if isempty(idx); idx = 1; end
    a = br_table(idx, 2);
    n = br_table(idx, 3);
    rd = a * Pc^n;
end


function [a, n] = get_br_params(Pc, br_table)
%GET_BR_PARAMS  Return burn rate coefficients for a given Pc.

    idx = find(br_table(:,1) <= Pc, 1, 'last');
    if isempty(idx); idx = 1; end
    a = br_table(idx, 2);
    n = br_table(idx, 3);
end


function grain = normalize_grain(grain, At_const)
%NORMALIZE_GRAIN  Ensure grain has br_table and At vector.
%
%   If grain uses scalar a_br/n_br, converts to single-row br_table.
%   If grain has no .At field, fills with constant At_const.

    % Burn rate table
    if ~isfield(grain, 'br_table')
        if isfield(grain, 'a_br') && isfield(grain, 'n_br')
            grain.br_table = [0, grain.a_br, grain.n_br];
        else
            error('Grain "%s" must have either br_table or a_br/n_br.', grain.name);
        end
    end

    % Throat area
    if ~isfield(grain, 'At') || isscalar(grain.At)
        if isfield(grain, 'At') && isscalar(grain.At)
            grain.At = grain.At * ones(size(grain.pct));
        else
            grain.At = At_const * ones(size(grain.pct));
        end
    end
end


function lbl = at_label(grain)
%AT_LABEL  Return a descriptive string for the grain's At type.

    if all(grain.At == grain.At(1))
        lbl = sprintf('constant (%.2e m^2)', grain.At(1));
    else
        lbl = sprintf('variable (%.2e - %.2e m^2)', min(grain.At), max(grain.At));
    end
end


%% ======================================================================
%  DATA LOADER — REPLACE THIS WITH YOUR REAL DATA
%  ======================================================================

function [sus, p2] = load_grain_data(At_const, Cstar)
%LOAD_GRAIN_DATA  Generate or load isolated grain data.
%
%   Returns two grain structs, each on their own independent %burn grid.
%
%   ---------------------------------------------------------------
%   BURN RATE SPECIFICATION:
%     Option A — Scalar (constant a, n):
%       grain.a_br = 3.2e-5;
%       grain.n_br = 0.35;
%
%     Option B — Pressure-dependent (piecewise a, n):
%       grain.br_table = [
%           0,    3.20e-5, 0.35;   % Pc < 5 MPa
%           5e6,  2.80e-5, 0.38;   % 5 MPa <= Pc < 10 MPa
%           10e6, 2.50e-5, 0.40;   % Pc >= 10 MPa
%       ];
%       Each row: [Pc_lower_bound, a, n]. Sorted ascending.
%
%   THROAT AREA SPECIFICATION:
%     Option A — Constant (omit .At; uses motor-level At_const):
%       (nothing needed)
%
%     Option B — Variable (provide .At as [N x 1] matching .pct):
%       grain.At = linspace(1.0e-3, 1.1e-3, N)';  % slight erosion
%   ---------------------------------------------------------------
%
%   REQUIRED OUTPUT FIELDS PER GRAIN:
%     .name, .pct, .Pc_iso, .mdot_iso, .rdot_iso, .Sburn, .rho,
%     .web, .dt, .t_cum
%   PLUS one of:
%     (.a_br, .n_br) OR .br_table
%   OPTIONAL:
%     .At  (if omitted, At_const is used)
%
%   If your data already has Pc and mdot, you only need to derive:
%     grain.rdot_iso(i) = eval_rdot(grain.Pc_iso(i), grain.br_table);
%     dw = grain.web * diff(grain.pct);
%     rdot_avg = 0.5*(rdot_iso(1:end-1) + rdot_iso(2:end));
%     grain.dt    = dw ./ rdot_avg;
%     grain.t_cum = [0; cumsum(grain.dt)];
%   ---------------------------------------------------------------

    % --- Sustain Grain (150 points, 3 burn rate regimes, variable At) ---
    N_sus    = 150;
    sus.name = 'Sustain';
    sus.rho  = 1750;
    sus.web  = 0.08;
    sus.pct  = linspace(0, 1, N_sus)';
    sus.Sburn = 0.30 * (1 - 0.05 * sin(pi * sus.pct));

    % Pressure-dependent burn rate (3 regimes)
    sus.br_table = [
        0,    3.20e-5, 0.35;   % low pressure regime
        5e6,  2.80e-5, 0.38;   % mid pressure regime
        10e6, 2.50e-5, 0.40;   % high pressure regime
    ];

    % Throat area: slight erosion over the burn
    sus.At = linspace(1.0e-3, 1.05e-3, N_sus)';

    sus = compute_isolated_equilibrium(sus, Cstar);

    % --- Pulse 2 Grain (300 points, single burn rate, constant At) ---
    N_p2    = 300;
    p2.name = 'Pulse 2';
    p2.rho  = 1750;
    p2.web  = 0.06;
    p2.pct  = linspace(0, 1, N_p2)';
    p2.Sburn = 0.18 * (1 + 0.10 * p2.pct - 0.10 * p2.pct.^2);

    % Single burn rate (scalar shorthand — will be converted to br_table)
    p2.a_br = 2.50e-5;
    p2.n_br = 0.38;

    % No .At field => will use At_const
    p2 = compute_isolated_equilibrium_scalar(p2, At_const, Cstar);
end


function grain = compute_isolated_equilibrium(grain, Cstar)
%COMPUTE_ISOLATED_EQUILIBRIUM  Derive Pc, rdot, mdot, dt using fzero.
%
%   Handles pressure-dependent burn rate (br_table) and variable At.
%   Solves: rho * Sburn * a(Pc) * Pc^n(Pc) = Pc * At(pct) / Cstar
%   at each %burn point using a bracketed root-finder.

    N = length(grain.pct);
    grain.Pc_iso   = zeros(N, 1);
    grain.rdot_iso = zeros(N, 1);
    grain.mdot_iso = zeros(N, 1);

    opts = optimset('TolX', 1e-8, 'Display', 'off');

    for i = 1:N
        At_i  = grain.At(i);
        Sb_i  = grain.Sburn(i);
        rho_i = grain.rho;
        brt   = grain.br_table;

        f = @(Pc) Sb_i * rho_i * eval_rdot(Pc, brt) - Pc * At_i / Cstar;

        % Initial guess from first row of br_table (closed form)
        a0 = brt(1, 2);  n0 = brt(1, 3);
        K = rho_i * Sb_i * a0 * Cstar / At_i;
        Pc_guess = K^(1 / (1 - n0));

        grain.Pc_iso(i)   = fzero(f, [1e3, max(Pc_guess * 5, 100e6)], opts);
        grain.rdot_iso(i) = eval_rdot(grain.Pc_iso(i), brt);
        grain.mdot_iso(i) = grain.Pc_iso(i) * At_i / Cstar;
    end

    dw       = grain.web * diff(grain.pct);
    rdot_avg = 0.5 * (grain.rdot_iso(1:end-1) + grain.rdot_iso(2:end));
    grain.dt    = dw ./ rdot_avg;
    grain.t_cum = [0; cumsum(grain.dt)];
end


function grain = compute_isolated_equilibrium_scalar(grain, At, Cstar)
%COMPUTE_ISOLATED_EQUILIBRIUM_SCALAR  Closed-form for constant a, n, At.
%
%   Uses Pc = (rho * Sburn * a * Cstar / At)^(1/(1-n)) directly.
%   This is the fast path when no pressure dependence or At variation.

    N = length(grain.pct);
    grain.Pc_iso   = zeros(N, 1);
    grain.rdot_iso = zeros(N, 1);
    grain.mdot_iso = zeros(N, 1);

    for i = 1:N
        K = grain.rho * grain.Sburn(i) * grain.a_br * Cstar / At;
        grain.Pc_iso(i)   = K^(1 / (1 - grain.n_br));
        grain.rdot_iso(i) = grain.a_br * grain.Pc_iso(i)^grain.n_br;
        grain.mdot_iso(i) = grain.Pc_iso(i) * At / Cstar;
    end

    dw       = grain.web * diff(grain.pct);
    rdot_avg = 0.5 * (grain.rdot_iso(1:end-1) + grain.rdot_iso(2:end));
    grain.dt    = dw ./ rdot_avg;
    grain.t_cum = [0; cumsum(grain.dt)];
end
