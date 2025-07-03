#include "config.h"
#include "DatManager.h"
#include "DacManager.h"
#include "PedestalManager.h"
#include "DataQualityChecker.h"
#include <fstream>

using namespace std;

void Config::Parse(const string config_file)
{
	conf = YAML::LoadFile(config_file);
	//Printing version number
	cout<<"HBUANA Version: "<<conf["hbuana"]["version"].as<std::string>()<<endl;
	cout<<"HBUANA Github Repository: "<<conf["hbuana"]["github"].as<std::string>()<<endl;
}

int Config::Run()
{
	if(conf["DataQualityCheck"]["on-off"].as<bool>())
	{
		cout<<"Data Quality Check mode: ON"<<endl;
		if(conf["DataQualityCheck"]["file-list"].as<std::string>()=="")
		{
			cout<<"ERROR: Please specify file list for data quality check"<<endl;
			return 0;
		}
		else
		{
			ifstream file_list(conf["DataQualityCheck"]["file-list"].as<std::string>());
			if(!file_list.is_open())
			{
				cout<<"ERROR: Cannot open file list "<<conf["DataQualityCheck"]["file-list"].as<std::string>()<<endl;
				return 0;
			}
			while(!file_list.eof())
			{
				string file_temp;
				file_list >> file_temp;
				if(file_temp=="")continue;
				cout<<"Analyzing file: "<<file_temp<<endl;
				DataQualityChecker dqchecker;
				dqchecker.AnalyzeFile(file_temp);
				dqchecker.PrintSummary();
				std::string filename = file_temp.substr(file_temp.find_last_of('/') + 1);
				filename = filename.substr(0, filename.find_last_of('.'));
				std::string outputfilename = conf["DataQualityCheck"]["output-dir"].as<std::string>() + "/" + filename + "_data_quality_report.txt";
				dqchecker.WriteReport(outputfilename);
				cout<<"Report written to: "<<outputfilename<<endl;
				// dqchecker.Clear(); // Clear the checker for the next file
			}
		}
	}
	if(conf["DAT-ROOT"]["on-off"].as<bool>())
	{
		cout<<"DAT mode: ON"<<endl;
		if(conf["DAT-ROOT"]["auto-gain"].as<bool>())cout<<"auto gain mode: ON"<<endl;//<<(conf["DAT-ROOT"]["auto-gain"].as<bool>())<<endl;	
		if(conf["DAT-ROOT"]["cherenkov"].as<bool>())cout<<"cherenkov detector: ON"<<endl;//<<(conf["DAT-ROOT"]["auto-gain"].as<bool>())<<endl;	
		if(conf["DAT-ROOT"]["file-list"].as<std::string>()=="" || conf["DAT-ROOT"]["output-dir"].as<std::string>()=="")
		{
			cout<<"ERROR: Please specify file list or output-dir for dat files"<<endl;
		}
		else
		{
			ifstream dat_list(conf["DAT-ROOT"]["file-list"].as<std::string>());
			DatManager dm;
			while(!dat_list.eof())
			{
				string dat_temp;
				dat_list >> dat_temp;
				if(dat_temp=="")continue;
				dm.Decode(dat_temp,conf["DAT-ROOT"]["output-dir"].as<std::string>(),conf["DAT-ROOT"]["auto-gain"].as<bool>(),conf["DAT-ROOT"]["cherenkov"].as<bool>());
			}
		}
	}
	if(conf["Pedestal"]["on-off"].as<bool>())
	{
		PedestalManager::CreateInstance();
		cout<<"Pedestal mode: ON"<<endl;
		if(conf["Pedestal"]["Cosmic"]["on-off"].as<bool>())
		{
			cout<<"Pedestal mode for cosmic events: ON"<<endl;
			_instance->Init(conf["Pedestal"]["Cosmic"]["output-file"].as<string>().c_str());
			_instance->Setmt(conf["Pedestal"]["Cosmic"]["usemt"].as<bool>());
			_instance->AnaPedestal(conf["Pedestal"]["Cosmic"]["file-list"].as<std::string>(),0);
			PedestalManager::DeleteInstance();
		}
		if(conf["Pedestal"]["DAC"]["on-off"].as<bool>())
		{
			cout<<"Pedestal mode for DAC events: ON"<<endl;
			_instance->Init(conf["Pedestal"]["DAC"]["output-file"].as<string>().c_str());
			_instance->AnaPedestal(conf["Pedestal"]["DAC"]["file-list"].as<std::string>(),1);
			PedestalManager::DeleteInstance();
		}
	}
	if(conf["Calibration"]["on-off"].as<bool>())
	{
		if(conf["Calibration"]["Cosmic"]["on-off"].as<bool>())
		{
			cout<<"Cosmic calibration mode:ON"<<endl;
			DacManager dacmanager("cosmic_calib.root");
			dacmanager.SetPedestal(conf["Calibration"]["Cosmic"]["ped-file"].as<string>().c_str());
			dacmanager.AnaDac(conf["Calibration"]["Cosmic"]["file-list"].as<std::string>(),"cosmic");
		}
		if(conf["Calibration"]["DAC"]["on-off"].as<bool>())
		{
			cout<<"DAC Calibration mode:ON"<<endl;
			DacManager dacmanager("dac_calib.root");
			dacmanager.SetPedestal(conf["Calibration"]["DAC"]["ped-file"].as<string>().c_str());
			dacmanager.AnaDac(conf["Calibration"]["DAC"]["file-list"].as<std::string>().c_str(),"dac");
		}
	}
	return 1;
}

void Config::Print()
{
	YAML::Node example_config = YAML::LoadFile("/home/diaohb/software/cepc_hbuana/config/config.yaml");
	ofstream fout("./config.yaml");
	fout << example_config;
	fout.close();
}


Config::Config() : conf(0)
{
}

Config::~Config()
{
}
