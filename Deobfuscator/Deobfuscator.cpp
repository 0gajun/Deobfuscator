// Deobfuscator

#include "stdafx.h"
#include "pe.h"
#include "trace.h"
#include "disassembler.h"
#include <iostream>

void deobfuscate(std::string original_pe_path,
	std::string trace_file_path,std::string output_binary_path)
{
	// Read
	PEReader::Builder builder;
	PEReader *reader = builder.setInputFilePath(original_pe_path)->build();
	if (reader == nullptr) {
		std::cout << builder.getErrMsg() << std::endl;
		exit(1);
	}
	PEFormat pe_fmt = reader->getPEFormat();

	std::unique_ptr<DisassembleResult> dis_res = std::make_unique<DisassembleResult>(Disassembler().disasmPE(pe_fmt));
	
	for (unsigned int addr : dis_res->insns_addr_set) {
		std::cout << "0x" << std::hex << addr << std::endl;
	}
	
	// Analyze
	TraceReader trace_reader;
	trace_reader.openTraceFile(trace_file_path);
	std::unique_ptr<TraceData> data = trace_reader.read();
	data->oep = pe_fmt.nt_headers.OptionalHeader.AddressOfEntryPoint;
	TraceAnalyzer analyzer(std::move(data), pe_fmt);
	analyzer.setDisassembleResult(std::move(dis_res));
	std::unique_ptr<TraceAnalysisResult> analysis_result = analyzer.analyze();

	// Edit
	PEEditor editor(pe_fmt, std::move(analysis_result));
	PEFormat *modified_pe = editor.result();

	// Write
	PEWriter::Builder writer_builder;
	PEWriter *writer = writer_builder
		.setOutputFilePath(output_binary_path)
		->setPEFormat(modified_pe)
		->build();
	if (writer == nullptr) {
		std::cout << writer_builder.getErrMsg() << std::endl;
		exit(1);
	}
	writer->write();
	return;
}

int main(int argc, char** argv)
{
	// TODO: use cmd_line option, and parse it
	std::cout << argc << std::endl;
	return 0;

	if (argc == 4) {
		deobfuscate(argv[1], argv[2], argv[3]);
	}
	else {
		std::string original_pe_path
			//= "C:\\Users\\ogamal\\Documents\\binary_code_obfuscation_sample\\bin\\overlap_func.exe";
			= "C:\\Users\\ogamal\\Documents\\binary_code_obfuscation_sample\\single_overlap_func.exe";
		std::string trace_file_path
			//= "C:\\Users\\ogamal\\Documents\\trace_data\\overlapping_functions.log";
			= "C:\\Users\\ogamal\\Documents\\trace_data\\single_overlaping_func.log";
		std::string output_binary_path
			= "C:\\Users\\ogamal\\Documents\\hoge_single_overlap.exe";
		deobfuscate(original_pe_path, trace_file_path, output_binary_path);
	}
}