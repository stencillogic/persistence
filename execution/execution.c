#include "execution/execution.h"

// statement execution depends on statement type
// select:
//   1. parser parses input: checks syntax and semantics
//   2. parser builds AST (long values can be saved on the disk)
//   3. existence of the objects and access privilages are checked
//   4. optimizer builds execution plan
//   5. executor executes statement according to plan: access rows, filters, joins sets,
//      saves intermediate results to disk, and spills resulting rows to client
// insert:
//   1. if insert is insert as select (not supported yet) then then the execution is
//      similar to select statement but resulting rows are spilled into table
//   2. if insert is insert values statement then statement is parsed up to values keyword
//      after that existence of target table and access rights are checked.
//      If check is successful parser parses values section and directly writes values to
//      database.
// update, delete:
//   similar to select but there is no resulting rows to spill
// create, alter, drop:
//   similar to update/delete but transaction is commited before statement execution


// execute statement
sint8 execution_exec_statement()
{
    parser_ast ast;
    if(parser_parse(&ast))
    {
        return 1;
    }
    return 0;
}
