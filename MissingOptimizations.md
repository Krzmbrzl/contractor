# Missing optimizations

In the following we list optimizations that could be made but the program does not yet discover and/or uses.

## CC2 (energy)

When calculating the CC2 energy there is the term `ECC2 += 0.5 * H[i⁺j⁺a⁻b⁻](||||) T1[a⁺i⁻](||) T1[b⁺j⁻](||)` that currently is factorized (after
density-fitting is introduced) as
```
ECC2 += -0.5 * B[i⁺a⁻q](||.) B[j⁺b⁻q](||.) T1[b⁺i⁻](||) T1[a⁺j⁻](||) factorizes to
B_T1[i⁺j⁻q](||.) += B[i⁺a⁻q](||.) T1[a⁺j⁻](||)
-> N_P^1 N_H^2 N_Q^1
B_B_T1[i⁺a⁻](||) += B[j⁺a⁻q](||.) B_T1[i⁺j⁻q](||.)
-> N_P^1 N_H^2 N_Q^1
ECC2 += -0.5 * B_B_T1[i⁺a⁻](||) T1[a⁺i⁻](||)
-> N_P^1 N_H^1
```
However if the factorizer was able to recognize that the two possible B_T1 terms were one and the same thing, this could produce the more optimal
factorization into
```
ECC2 += -0.5 * B[i⁺a⁻q](||.) B[j⁺b⁻q](||.) T1[b⁺i⁻](||) T1[a⁺j⁻](||) factorizes to
  B_T1[i⁺j⁻q](||.) += B[i⁺a⁻q](||.) T1[a⁺j⁻](||)
  ECC2 += -0.5 * B_T1[i⁺j⁻q](||) B_T1[j⁺i⁻q](||)
```

Even more advanced (and even harder to detect automatically) would be (for the entire correlation energy - not only the "antisymmetric half"):
```
C[abij] += T[abij] 
C[abij] += T[ai]T[bj]
Y[Qai] += DF[Qck] (2 C[acik] - C[caik])
ECC += DF[Qai]Y[Qai]
```


# CCD (residual)

In CCD there is one diagram that currently produces
```
>>>> O2-u[a⁺b⁺i⁻j⁻](||||) += 0.5 * H[k⁺l⁺c⁻d⁻](||||) T2[a⁺c⁺i⁻k⁻](||||) T2[b⁺d⁺j⁻l⁻](||||) <<<<
# of Terms: 3
- 1: {
  H_T2[i⁺a⁺b⁻j⁻](/\/\) += 2 * H[i⁺k⁺b⁻c⁻](....) T2[a⁺c⁺j⁻k⁻](....)
  H_T2[i⁺a⁺b⁻j⁻](/\/\) += - H[i⁺k⁺b⁻c⁻](....) T2[a⁺c⁺k⁻j⁻](....)
  H_T2[i⁺a⁺b⁻j⁻](/\/\) += - H[i⁺k⁺c⁻b⁻](....) T2[a⁺c⁺j⁻k⁻](....)
}
- 2: {
  H_T2[i⁺a⁺b⁻j⁻](////) += 2 * H[i⁺k⁺b⁻c⁻](....) T2[a⁺c⁺j⁻k⁻](....)
  H_T2[i⁺a⁺b⁻j⁻](////) += - H[i⁺k⁺b⁻c⁻](....) T2[a⁺c⁺k⁻j⁻](....)
  H_T2[i⁺a⁺b⁻j⁻](////) += - H[i⁺k⁺c⁻b⁻](....) T2[a⁺c⁺j⁻k⁻](....)
  H_T2[i⁺a⁺b⁻j⁻](////) += H[i⁺k⁺c⁻b⁻](....) T2[a⁺c⁺k⁻j⁻](....)
}
- 3: {
  O2-u[a⁺b⁺i⁻j⁻](....) += 0.5 * H_T2[k⁺a⁺c⁻i⁻](////) T2[b⁺c⁺j⁻k⁻](....)
  O2-u[a⁺b⁺i⁻j⁻](....) += 0.5 * H_T2[k⁺a⁺c⁻i⁻](/\/\) T2[b⁺c⁺j⁻k⁻](....)
  O2-u[a⁺b⁺i⁻j⁻](....) += -0.5 * H_T2[k⁺a⁺c⁻i⁻](/\/\) T2[b⁺c⁺k⁻j⁻](....)
}
```

A more optimal way of calculating term #2 would be:
```
H_T2[i⁺a⁺b⁻j⁻](////) += H_T2[i⁺a⁺b⁻j⁻](/\/\)
H_T2[i⁺a⁺b⁻j⁻](////) += H[i⁺k⁺c⁻b⁻](....) T2[a⁺c⁺k⁻j⁻](....)
```
That means the first three contractions can be saved by recognizing that these are exactly what already has been calculated for
`H_T2[i⁺a⁺b⁻j⁻](/\/\)`.

