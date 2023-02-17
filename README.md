# SpreadSheet___

Primitive spreadsheet engine for parsing and evaluating basic expressions

## Examples : 

- Simple expression evaluation : 
```
| A         | B         |
| 5         |5          |
| 3         | 0         |
| =A1+B1    |=A2+B2     |
```
Parsed and evaluated to : 
```
| A         | B         |
| 5         |5          |
| 3         | 0         |
| 10        |3          |
```
- Clone `meta-command`: parsed before evaluation to the corresponding adjacent cell with shift if needed of cell refreneces in expressions
```
| A         | B         |
| 0         | 5         |
| =A1+2     | :<        |
| :^        | :^        |
```

parsed to something equivalent to :
```
| A         | B         |
| 0         | 5         |
| =A1+2     | =B1+2     |
| =A2+2     | =B2+2     |
```

and evaluated to : 
```
A       |B
0.000000|5.000000
2.000000|7.000000
4.000000|9.000000
```

