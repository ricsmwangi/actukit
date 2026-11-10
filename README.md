# actukit

[![CI](https://github.com/ricsmwangi/actukit/actions/workflows/ci.yml/badge.svg)](https://github.com/ricsmwangi/actukit/actions/workflows/ci.yml)
[![Python](https://img.shields.io/badge/python-3.10%2B-blue)](https://www.python.org)
[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](LICENSE)

A mini actuarial data-science toolkit. Fast **C/C++** core for loop-heavy math,
**SQL** for storage, **Python** for orchestration and plotting.

## Layers

- **C** — streaming I/O for huge exposure / triangle feeds
- **C++** — life tables, annuities, chain-ladder + Mack standard errors
- **SQL** — mortality tables, exposure, claims triangles
- **Python** — load, query, plot, orchestrate

## Validated against Mack (1993)

The chain-ladder engine is tested against the canonical reference triangle from
Mack (1993), *Distribution-Free Calculation of the Standard Error of IBNR
Reserves*, ASTIN Bulletin 23(2):

| Quantity          | Published      | This repo      |
|-------------------|----------------|----------------|
| Total reserve     | 18,680,856     | 18,680,856     |
| Standard error    |  2,447,098     |  2,447,098     |

`pytest` locks these numbers — any regression breaks CI.

## Build

```bash
