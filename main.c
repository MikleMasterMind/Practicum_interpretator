#include <stdio.h>
#include <ctype.h>
#include <stdlib.h>
#include <string.h>

#define true  1
#define false 0
#define Nan 2
#define MAXLENGTH 101 // guaranteed maximum string length

// store all numbers in this form
struct number {
    double value;
    int is_float;
    char name[MAXLENGTH];
};
typedef struct number num;

// for list to store variables
struct variable {
    num data;
    struct variable* next;
};
typedef struct variable var;

// for stack to count expression
struct operand {
    num data;
    struct operand* prev;
};
typedef struct operand operand;

// for stack to get prefix notation
struct Node {
    char data;
    struct Node* prev;
};
typedef struct Node node;

// show list
void print_list(const var* list) {
    if (list == NULL) {
        return;
    }
    while (list != NULL) {
        // printf(list->data.name);
        for (int i = 0; list->data.name[i] != '\0'; ++i) {
            putchar(list->data.name[i]);
        }
        printf(" = ");
        if (list->data.is_float == true) {
            printf("%f", list->data.value);
        } else {
            printf("%d", (int)list->data.value);
        }
        printf(", ");
        list = list->next; 
    }
    printf("\n");
};

// check if variable is in list
int contain(const var* list, const char* name) {
    while (list != NULL) {
        if (strncmp(name, list->data.name, MAXLENGTH) == 0) {
            return true;
        } else {
            list = list->next;
        }
    }
    return false;
}

// add new variable to list
void add_var(var** list, const char* line, const int head, const int end) {

    int length = end - head + 1;
    char* new_name = (char*)calloc(length, sizeof(char));

    for (int i = head; i < end; ++i) {
        new_name[i-head] = line[i];
    }

    if (contain(*list, new_name)) {
        free(new_name);
        return;
    }

    var* tmp = (var*)malloc(sizeof(var));
    tmp->next = *list;

    if (isdigit(new_name[0]) || new_name[0] == '-') {
        tmp->data.value = strtod(new_name, NULL);
        if (strchr(new_name, '.') != NULL) {
            tmp->data.is_float = true;
        } else {
            tmp->data.is_float = false;
        }
    } else {
        tmp->data.value = 0.0;
        tmp->data.is_float = Nan;
    }
    strncpy(tmp->data.name, new_name, length);
    tmp->data.name[length] = '\0';

    free(new_name);
    *list = tmp;

};

// get value of variable "name"
num get(const var* list, const char* name) {
    num result;
    while (list != NULL) {
        if (strncmp(name, list->data.name, MAXLENGTH) == 0) {
            result =  list->data;
            break;
        } else {
            list = list->next;
        }
    }
    return result;
}

// sat value to variable in list
void set_value(var** list, const num value) {
    var* vars = *list;
    while (true) {
        if (strncmp(value.name, vars->data.name, MAXLENGTH) == 0) {
            break;
        } else {
            vars = vars->next;
        }
    }

    if (vars->data.is_float == Nan) {
        vars->data.is_float = value.is_float;
    }

    if (vars->data.is_float == true) {
        vars->data.value = value.value;
    } else {
        vars->data.value = (int)value.value;
    }
};

// delete variable form list
void delete_var(var** list, const char* name) {

    if (contain(*list, name)) {

        var* prev = NULL, * elem = *list;

        if ((*list)->next == NULL) {
            // list contain 1 element  
            *list = NULL;
            free(elem);
            return;
        } else {

            // find var to delete
            while (true) {
                if (strncmp(name, elem->data.name, MAXLENGTH) == 0) {
                    break;
                } else {
                    prev = elem;
                    elem = elem->next;
                }
            }

            // first elem to delete
            if (prev == NULL) {
                *list = elem->next;
            } else { // other
                prev->next = elem->next;
            }
            free(elem);
        }
    }
};

// delete all constant
void delete_const(var** list) {
    var* elem = *list;
    while (elem != NULL) {
        if (isdigit(elem->data.name[0]) || elem->data.name[0] == '-') {
            delete_var(list, elem->data.name);
        }
        elem = elem->next;
    }
}

// clean memory
void delete_list(var* list) {
    var* tmp;
    while (list != NULL) {
        tmp = list;
        list = list->next;
        free(tmp);
    }
}

// symbol can't be in variable name
int end_of_name(const char c) {
    switch (c)
    {
    case ('+'): case ('-'): case('*'): case ('/'):case ('%'): case(')'): case('='): case(' '): case('\0'): case('\n'):
        return true;
        break;
    default:
        return false;
        break;
    }
}

// first symbols of variable name
int begin_of_name(const char c) {
    return isalnum(c) || c == '_' || c == '-';
};

// push_char new item to stack
void push_char(node** stack, const char data) {
    node* tmp = (node*)malloc(sizeof(node));
    tmp->data = data;
    tmp->prev = *stack;
    *stack = tmp;
};

// get last item from stack
char pop_char(node** stack) {
    char tmp = (*stack)->data;
    node* free_me = *stack;
    (*stack) = (*stack)->prev;
    free(free_me);
    return tmp;
};

// clan memory
void delete_stack_char(node* stack) {
    node* tmp;
    while(stack != NULL) {
        tmp = stack;
        stack = stack->prev;
        free(tmp);
    }
};

// get prefix notation and highlight variable by #
// a = b * c + d -> #a#=#b##c#*#d#+
void make_prefix(char line[], var** vars) {

    // stack initialisation
    node* stack = NULL;
    push_char(&stack, '(');

    char newline[MAXLENGTH]; // rewrinte line in this line
    
    int i, head, end, j = 0; // index
    int binary_minus = false;
    char buf; // buffer

    for (i = 0; line[i] != '\0';) {
        
        // write variable name: var -> #var#
        // get name and store it in list vars
        if (begin_of_name(line[i]) && !binary_minus) {
            newline[j++] = '#';
            head = i; // head of name
            newline[j++] = line[i++];
            for( ;!end_of_name(line[i]); ++i) {
                newline[j++] = line[i];
            }
            end = i; // end of name
            add_var(vars, line, head, end);
            newline[j++] = '#';
            binary_minus = true; // after var may be only binary minus
            
            continue;
        }

        switch (line[i])
        {
        case ('('):
            push_char(&stack, line[i]);
            binary_minus = false; // binary minus can't be agter '('
            break;
        
        case (')'):
            buf = pop_char(&stack);
            while(buf != '(') {
                newline[j++] = buf;
                buf = pop_char(&stack);
            }
            binary_minus = true; // after ')' may be only binary minus
            break;

        case ('-'): case ('+'): //same cases (unary minus processed befor)
            buf = pop_char(&stack);
            while (buf == '*' || buf == '/' || buf == '%') {
                newline[j++] = buf;
                buf = pop_char(&stack);
            }
            push_char(&stack, buf);
            push_char(&stack, line[i]);
            binary_minus = false; // only unary minus can be after '+' and '-'
            break;

        case ('*'): case ('/'): case ('%'): // same cases
            push_char(&stack, line[i]);
            binary_minus = false; // only unary minus can be next
            break;

        case ('='):
            buf = pop_char(&stack);
            while (buf != '(') {
                newline[j++] = buf;
                buf = pop_char(&stack);
            }
            push_char(&stack, buf);
            push_char(&stack, line[i]);
            binary_minus = false; // only unary minus can be next

            break;

        default:
            break;
        }
        ++i;
    }

    
    buf = pop_char(&stack);
    while(buf != '(') {
        newline[j++] = buf;
        buf = pop_char(&stack);
    }

    delete_stack_char(stack);

    strncpy(line, newline, j);

    line[j] = '\0';


};

// push to operand_stack
void push_operand(operand** stack, const num data) {
    operand* tmp = (operand*)malloc(sizeof(operand));
    tmp->data = data;
    tmp->prev = *stack;
    *stack = tmp;
};

// pop from operand_stack
num pop_operand(operand** stack) {
    num tmp = (*stack)->data;
    operand* free_me = *stack;
    (*stack) = (*stack)->prev;
    free(free_me);
    return tmp;
};

// clean memory
void delete_stack_operand(operand* stack) {
    operand* tmp;
    while(stack != NULL) {
        tmp = stack;
        stack = stack->prev;
        free(tmp);
    }
};

//count value after =
num count(const char* line, const int index, var* vars) {

    // init stack
    operand* stack = NULL;
    num a, b;

    char* name = (char*)calloc(MAXLENGTH, sizeof(char));
    int j;

    for (int i = index; line[i] != '\0';) {
        switch(line[i]){
            case ('='):
                a = pop_operand(&stack);
                b = pop_operand(&stack);
                b.value = a.value;
                if (b.is_float == Nan) {
                    b.is_float = a.is_float;
                }
                push_operand(&stack, b);
                set_value(&vars, b);
                ++i;
                break;
            case ('#'):
                j = 0;
                ++i;
                while (line[i] != '#') {
                    name[j++] = line[i++];
                }
                ++i;
                name[j] = '\0';
                a = get(vars, name);
                push_operand(&stack, a);
                break;
            case ('+'):
                a = pop_operand(&stack);
                b = pop_operand(&stack);
                a.value = a.value + b.value;
                a.is_float = a.is_float || b.is_float;
                push_operand(&stack, a);
                ++i;
                break;
            case ('-'):
                a = pop_operand(&stack);
                b = pop_operand(&stack);
                a.value = b.value - a.value;
                a.is_float = a.is_float || b.is_float;
                push_operand(&stack, a);
                ++i;
                break;
            case ('*'):
                a = pop_operand(&stack);
                b = pop_operand(&stack);
                a.value = a.value * b.value;
                a.is_float = a.is_float || b.is_float;
                push_operand(&stack, a);
                ++i;
                break;
            case ('/'):
                a = pop_operand(&stack);
                b = pop_operand(&stack);
                if (a.is_float || b.is_float) {
                    a.value = b.value / a.value;
                    a.is_float = true;
                }  else {
                    a.value = (int)b.value / (int)a.value;
                    a.is_float = false;
                }
                push_operand(&stack, a);
                ++i;
                break;
            case ('%'):
                a = pop_operand(&stack);
                b = pop_operand(&stack);
                a.value = (int)b.value % (int)a.value;
                a.is_float = false;
                push_operand(&stack, a);
                ++i;
                break;

        }
    }

    free(name);

    a = pop_operand(&stack);
    set_value(&vars, a);
    delete_stack_operand(stack);

    return a;


};

int main() {

    var* vars = NULL;

    char line[MAXLENGTH];

    while (true) {
        putchar('>');
        
        #ifndef debug
        fgets(line, MAXLENGTH, stdin);
        #endif

        // special commands
        if (strncmp(line, "exit", 4) == 0) {

            printf("end program\n");
            delete_list(vars);
            break;

        } else if (strncmp(line, "del", 3) == 0) {

            char name[MAXLENGTH];
            int j = 0;
            for (int i = 3; line[i] != '\0' && line[i] != '\n'; ++i) {
                if (line[i] != ' ') {
                    name[j++] = line[i];
                }
            }
            name[j] = '\0';
            delete_var(&vars, name);

        } else {

            make_prefix(line, &vars);

            num a = count(line, 0, vars);

            if (a.is_float == true) {
                printf("Result: %f\n", a.value);
            } else {
                printf("Result: %d\n", (int)a.value);
            }

            delete_const(&vars);
        }
        print_list(vars);
    }
    return 0;   
}