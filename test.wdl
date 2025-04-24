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

      # Runtime parameters
      String gatk_docker
      Int? mem_gb
      Int? disk_space_gb
      Boolean use_ssd = false
      Int? cpu
      Int? preemptible_attempts
    }

    Int machine_mem_mb = select_first([mem_gb, 2]) * 1000
    Int command_mem_mb = machine_mem_mb - 500

    # Determine output filename
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