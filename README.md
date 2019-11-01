# Unnamed language

## Design

- Immutable and value semantics everywhere.

- No stop-the-world garbage collection; borrow checking maybe or something similar

- Static analysis will allow compiler to optimise with RVO etc

- Powerful autothreading also via static analysis

## Future

- Complete "reference" interpreter
- Proper JIT