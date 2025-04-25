# wdlrunner

**A faster Workflow Definition Language (WDL) Compiler and Runner written in C++**

`wdlrunner` is a performance-focused compiler and executor for WDL scripts, designed with fast parsing, accurate AST generation, and lightweight runtime execution in mind.

---

## Features

- Fast and minimal dependency C++ implementation
- Parses WDL 1.0 compliant code
- Converts WDL source into AST
- Supports variable scoping, expressions, task blocks, input/output declarations, command blocks, runtime, and parameter_meta
- Handles both optional and defaulted parameters
- Includes basic evaluation of expressions and string interpolations

---

## Specification

We support most WDL v1.0 features. Below is a breakdown of the current supported functionality.

### Supported

#### Language Elements
- `version` declaration
- `task` blocks
- `workflow` blocks *(basic parsing, execution planned)*

#### Input/Output
- Optional and required inputs (`File?`, `Int?`, etc.)
- Output declarations
- `select_first([...])` expression
- `default=...` inline default values

#### Expressions
- Arithmetic: `+`, `-`, `*`, `/`
- Boolean: `&&`, `||`, `!`
- String interpolation: `~{expression}`
- Function calls: `select_first`, `basename`, `default`

#### Declarations
- Variable declarations with and without initialization
- Complex expressions in variable initialization
- Inferred and explicit typing

#### Runtime
- `docker`, `memory`, `cpu`, `disks`, `preemptible`
- `if` expressions and inline evaluation logic

#### Metadata
- `parameter_meta` parsing
- Metadata fields like `localization_optional`

#### Command Blocks
- Multiline `command <<< >>>` support
- Embedded WDL expressions within commands

---

## Example

### Input WDL

```wdl
version 1.0

task PreprocessIntervals {
    input {
        File? intervals
        File? blacklist_intervals
        File ref_fasta
        File ref_fasta_fai
        File ref_fasta_dict
        Int? padding
        Int? bin_length
        File? gatk4_jar_override

        String gatk_docker
        Int? mem_gb
        Int? disk_space_gb
        Boolean use_ssd = false
        Int? cpu
        Int? preemptible_attempts
    }

    Int machine_mem_mb = select_first([mem_gb, 2]) * 1000
    Int command_mem_mb = machine_mem_mb - 500

    String filename = select_first([intervals, "wgs"])
    String base_filename = basename(filename, ".interval_list")

    parameter_meta {
        bam: {
            localization_optional: true
        }
        bam_idx: {
            localization_optional: true
        }
    }

    command <<< 
        set -eu
        export GATK_LOCAL_JAR=~{default="/root/gatk.jar" gatk4_jar_override}
        gatk --java-options "-Xmx~{command_mem_mb}m" PreprocessIntervals \
            ~{"-L " + intervals} \
            ~{"-XL " + blacklist_intervals} \
            --reference ~{ref_fasta} \
            --padding ~{default="250" padding} \
            --bin-length ~{default="1000" bin_length} \
            --interval-merging-rule OVERLAPPING_ONLY \
            --output ~{base_filename}.preprocessed.interval_list
    >>>

    runtime {
        docker: gatk_docker
        memory: machine_mem_mb + " MB"
        disks: "local-disk " + select_first([disk_space_gb, 40]) + if use_ssd then " SSD" else " HDD"
        cpu: select_first([cpu, 1])
        preemptible: select_first([preemptible_attempts, 5])
    }

    output {
        File preprocessed_intervals = "~{base_filename}.preprocessed.interval_list"
    }
}


```


## Full Generated AST Structure

```plaintext
N_PROGRAM
├─ N_VERSION_DECL 
└─ N_CLASS_DECL (PreprocessIntervals)
   ├─ N_INPUT_DECL
   │  └─ N_BLOCK (14 variables):
   │     ├─ N_VAR_DECL [Nullable] (intervals: File?)
   │     ├─ N_VAR_DECL [Nullable] (blacklist_intervals: File?)
   │     ├─ N_VAR_DECL (ref_fasta: File)
   │     ├─ N_VAR_DECL (ref_fasta_fai: File)
   │     ├─ ... (10 more input declarations)
   │     └─ N_VAR_DECL [Default] (use_ssd: Boolean = false)
   │
   ├─ VARIABLE DECLARATIONS:
   │  ├─ N_VAR_DECL (machine_mem_mb: Int)
   │  │  └─ N_BINARY_EXPR (select_first([mem_gb, 2]) * 1000)
   │  │     ├─ N_FUNC_CALL (select_first)
   │  │     │  └─ N_ARRAY [mem_gb, 2]
   │  │     └─ N_LITERAL (1000)
   │  │
   │  ├─ N_VAR_DECL (command_mem_mb: Int)
   │  │  └─ N_BINARY_EXPR (machine_mem_mb - 500)
   │  │
   │  ├─ N_VAR_DECL (filename: String)
   │  │  └─ N_FUNC_CALL (select_first)
   │  │     └─ N_ARRAY [intervals, "wgs"]
   │  │
   │  └─ N_VAR_DECL (base_filename: String)
   │     └─ N_FUNC_CALL (basename)
   │        ├─ N_IDENT (filename)
   │        └─ N_LITERAL (".interval_list")
   │
   ├─ N_META_DECL (Partial Implementation)
   │  ├─ N_META_MEMBER_DECL (bam: {localization_optional: true})
   │  └─ N_META_MEMBER_DECL (bam_idx: {localization_optional: true})
   │
   ├─ N_COMMAND_DECL
   │  └─ N_LITERAL (Raw command text)
   │
   ├─ N_RUNTIME_DECL (Partial Evaluation)
   │  ├─ docker: N_IDENT (gatk_docker)
   │  ├─ memory: N_BINARY_EXPR (machine_mem_mb + " MB")
   │  ├─ disks: Complex concatenation with:
   │  │  ├─ N_LITERAL ("local-disk ")
   │  │  ├─ N_FUNC_CALL (select_first)
   │  │  └─ N_IF_STMT (use_ssd ? " SSD" : " HDD")
   │  ├─ cpu: N_FUNC_CALL (select_first)
   │  └─ preemptible: N_FUNC_CALL (select_first)
   │
   └─ N_OUTPUT_DECL
      └─ N_VAR_DECL (preprocessed_intervals)
         └─ N_LITERAL (String with interpolation)

```

## ✅ TODO

Here's a list of currently planned or partially implemented features:

- [ ] **Workflow block support:** Execution of `workflow` blocks and task chaining
- [ ] **Struct Definition:** And Custom Types...
- [ ] **Imports**
- [ ] **Call Statement/Expression**
- [ ] **Map[String,String]**
- [ ] **Scatter/Gather** - It is loops we have loops already...wth
- [ ] **Optional Parameters** call_func(default='default_value_here', optional_param)
- [ ] **Scatter and if blocks:** Conditional and parallel executions
- [ ] **Advanced error handling:** Type checking and better diagnostics
- [ ] **Imports and namespaces:** Support for `import` statements and reusable modules
- [ ] **Library expansion:** More functions like `length`, `size`, `sub`, etc.
- [x] **CLI tool:** A runner to compile and execute WDL scripts directly
- [ ] **Testing:** Unit and integration tests for all components
