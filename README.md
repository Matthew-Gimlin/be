# 🐝 be

`be` is a compiler backend.

## Build

Build `be` using `make`.

```sh
make
```

## Example

Say `example.ir` contains the following code.

```
func i @main() {
entry:
    %0 = i eq 0, 1
    cbr i %0, true, false

true:
    ret i 0

false:
    ret i 1

unreachable:
    ret i 2
}
```

Run `be` with the file.

```sh
./be example.ir
```

`be` does not yet emit assembly. However, it does produce *slightly more*
optimized IR. For the example above, the output appears as follows.

```
func i @main() {
entry:
  ret i 1
}
```
