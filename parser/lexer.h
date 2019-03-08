#ifndef _LEXER_H
#define _LEXER_H

/*
Grammatics:
[] - not mandatory
{} - zero or more times
() - grouping
<low_case> - nonterminal lexem
others - terminals (optionally in double quotes)

<sql_statement> := <statement_body> ;

<statement_body> := <select_statement> |
                    <insert_statement> |
                    <update_statement> |
                    <delete_statement> |
                    <create_statement> |
                    <drop_statement> |
                    <alter_statement>

<select_statement> := <single_select_statement>
                      [ { ( UNION | INTERSECT | EXCEPT ) [ ALL ] <single_select_statement> } ]
                      [ ORDER BY <order_by_expr_list> ]

<order_by_expr_list> := <expression_with_sort> [ {, <expression_with_sort> } ]
<expression_with_sort> := <expression> [ ASC | DESC ] [ NULLS ( FIRST | LAST ) ]

Arithmetic (or maybe string) expression with column names, literals, other identifiers:

<expression> := [ + | - ] <term> { [ + | - ] <term> }
<term> := <multiplier> { [ * | / ] <multiplier> }
<multiplier> := <literal> | <name> | "(" <expression> ")"

<single_select_statement> := SELECT [ ALL | DISTINCT ] <projection>
                             [ FROM <from> ]
                             [ WHERE <conditional_expr> ]
                             [ GROUP BY <expression_list> ]
                             [ HAVING <predicates> ]

<projection> := * | <expression_list>
<expression_list> := <named_expression> [ {, <named_expression> } ]
<named_expression> := <expression> [ [ AS ] <name> ]

<from> := <from_item> [ ( [ INNER ] | [ CROSS ] | [ LEFT [ OUTER ] ] | [ RIGHT [ OUTER ] | FULL [ OUTER ] ] ) JOIN <from_item> [ ON <conditional_expr> ] ]

<from_item> := <name> [ [ AS ] <name> ] |
              "(" <select_statement> ")" [ [ AS ] <name> ]

<conditional_expr> := <boolean_term> |
                      <conditional_expr> OR <boolean_term>

<boolean_term> := <boolean_factor> |
                  <boolean_term> AND <boolean_factor>

<boolean_factor> := [ NOT ] <boolean_test>
<boolean_test> := <predicate> | "(" <conditional_expr> ")"
<predicate> := <expression> ( > | < | >= | <= | = | <> ) <expression> | <expression> IS NULL

*/

#endif
