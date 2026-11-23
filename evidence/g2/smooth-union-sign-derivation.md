# G2 — smooth-union sign unification (SPEC v2.1 §6.2 note)

Status: **DERIVED AND TESTED** — both `fusion::sdSmoothUnion` and
`fusion::sdSmoothSubtraction` in `SdfFusion.cpp` were corrected after the
derivation below and after a failing dual-implementation test was in place.

## The conflict recorded in the SPEC

> 符号约定：`SdfFusion.cpp:53` 为 `+ k*h*(1-h)`；`AnatomyField.cpp:109` 为
> `- k*h*(1-h)`；均声称 negative inside。差异真实但生产路径只用 AnatomyField
> 版（`fusion::sdSmoothUnion` 仅测试调用）→ 休眠缺陷，禁止猜改符号，须先数学
> 推导 + 双实现单测统一（列入 G2）。

The SPEC forbids guessing the sign. Nothing below was decided by inspection.

## Derivation

Let `a`, `b` be negative-inside distance fields with unit gradient, and `k > 0`
the blend radius. Define

```
h = clamp(1/2 + (b - a) / (2k), 0, 1)
f = b(1-h) + a*h + c*k*h*(1-h)
```

Inside the blend band `a - b = k(1-2h)`. Differentiating (with
`dh/da = -1/(2k)`, `dh/db = +1/(2k)`):

```
df/da = h     - (1+c)(1-2h)/2
df/db = (1-h) + (1+c)(1-2h)/2
df/da + df/db = 1                    (for every c)
```

For `f` to remain a distance *bound*, both partials must stay in `[0,1]`:

| c | df/da | df/db | verdict |
|---|---|---|---|
| `-1` | `h` | `1-h` | both in `[0,1]` — **valid** |
| `+1` | `3h-1` | `2-3h` | negative / greater than 1 when `h < 1/3` — **invalid** |

Value where the two surfaces coincide (`a = b = 0`, hence `h = 1/2`):

| c | f | meaning |
|---|---|---|
| `-1` | `-k/4` | inside — the union **bulges outward** (correct smooth union) |
| `+1` | `+k/4` | outside — the union **shrinks** (that is a carve) |

Conclusion: a negative-inside smooth union requires `c = -1`, i.e.
`- k*h*(1-h)`. `AnatomyField.cpp:107-110` already had this form and was correct.

### Subtraction is the opposite sign

`a ∖ b = a ∩ ¬b`, so its SDF is `max(a, -b)`, and its smooth form is
`smoothMax(a, -b) = -smoothMin(-a, b)`. In the subtraction code's own `h`
convention this gives `+ k*h*(1-h)`. At coincident surfaces the subtraction
must carve (`f = +k/4`, outside), not add material. `AnatomyField.cpp:112-116`
already had `+` and was correct; `SdfFusion.cpp` had `-`.

So **both** fusion utilities were inverted, not just the union named in the
SPEC note. Both were corrected.

## Dual-implementation test

`tests/unit/smooth_union_sign_tests.cpp` contains two independently written
references:

1. the derived polynomial written out directly;
2. `smoothMax(a,b) = -smoothMin(-a,-b)` — an algebraic identity used to write
   the same operation a second way.

It checks both fusion functions against both references over
`a,b ∈ {-0.4,-0.15,-0.02,0,0.02,0.15,0.4}` and `k ∈ {0.02,0.1,0.3,1.0}`,
plus the defining properties:

- coincident union is negative and exactly `-k/4`;
- outside the blend band the field equals the plain union exactly;
- the numerical gradient stays in `[0,1]` and `df/da + df/db = 1`;
- coincident subtraction is positive and exactly `+k/4`.

**Before the fix:** 4 cases, 212 failures.
**After the fix:** 4 cases, 0 failures; all 11 host tests pass.

## Blast radius

- `fusion::sdSmoothUnion` / `sdSmoothSubtraction` had **no production caller** —
  only `tests/unit/fusion_tests.cpp` and the new sign test call them. The
  shipped anatomy surface is built by `anatomy::makeUpperBodyField`, which
  calls `AnatomyField.cpp`'s own (already correct) functions.
- The edit therefore changes compiled bytes but **not app runtime behaviour**.
- Existing fusion tests (two-sphere merge, shoulder blend, socket carve) all
  still pass without weakening any assertion.
- APK SHA256: DEF-01 / G1 build `088476fcd1fd5b84edfe7109116f310a4753a8a627a837fe39691320f055a9c4`
  -> post-fix build `c1cace9d41c62ce442f09b2005f33a42b5899a81491bf52ba5fe1c3525d80ae9`.
  Both APKs are 60,187,133 bytes. The hash changed because `SdfFusion.cpp` is
  compiled into the production library; it does not by itself imply a runtime
  difference, since neither function has a production caller.
- On-device re-verification after this edit: **NOT RUN** at G2. CPU/GPU
  numerical parity remains `[UNVERIFIED]` and is explicitly NOT a kill
  condition (SPEC §11.1); this is carried forward as a G3/G4 risk.
