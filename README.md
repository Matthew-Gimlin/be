# be

`be` is a compiler backend.

## Build

Build `be` using `make`.

```sh
make
```

## Example

Say `example.ir` contains the following code.

```
func i @sum(i %0, i %1, i %2) {
entry:
  %3 = add i %0, %1
  %4 = add i %2, %3
  ret i %4
}

func i @main() {
entry:
  %sum = call i @sum(1, 2, 3)
  %comparison = eq i %sum, 6
  cbr i %comparison, ok, error

ok:
  ret i 0

error:
  ret i 1
}
```

Run `be` with the file.

```sh
./be example.ir
```

`be` does not yet emit assembly. However, it does produce *slightly more*
optimized IR. For the example above, the output appears as follows.

```
func i @sum(i %0, i %1, i %2) {
entry:
  %3 = add i %0, %1
  %4 = add i %2, %3
  ret i %4
}

func i @main() {
entry:
  ret i 0
}
```
