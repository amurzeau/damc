#include "EqualizersController.h"
#include "Equalizer.h"
#include "OutputController.h"
#include "ui_EqualizersController.h"
#include <BiquadFilter.h>
#include <QFileDialog>
#include <QHideEvent>
#include <QMessageBox>
#include <QRegularExpression>
#include <QTextStream>

EqualizersController::EqualizersController(OutputController* parent, OscContainer* oscParent)
    : QDialog(parent), ui(new Ui::EqualizersController), oscEqualizers(oscParent, "eqFilters") {
	ui->setupUi(this);
	oscEqualizers.setWidget(this,
	                        ui->eqHorizontalLayout,
	                        [this, parent](QWidget* widgetParent, OscContainer* oscParent, int name) -> QWidget* {
		                        return new Equalizer(this, oscParent, std::to_string(name), parent, ui->bodePlot);
	                        });
	ui->menuButton->setMenu(&buttonMenu);
	ui->menuButton->setPopupMode(QToolButton::InstantPopup);

	QAction* importAction = new QAction("&Import EQ config", this);
	connect(importAction, &QAction::triggered, this, &EqualizersController::onImportAction);
	buttonMenu.addAction(importAction);

	QAction* exportAction = new QAction("&Export EQ config", this);
	connect(exportAction, &QAction::triggered, this, &EqualizersController::onExportAction);
	buttonMenu.addAction(exportAction);
}

EqualizersController::~EqualizersController() {
	delete ui;
}

bool EqualizersController::getEqEnabled() {
	bool eqEnabled = false;
	for(auto item : oscEqualizers.getWidgets()) {
		Equalizer* eq = (Equalizer*) item;
		if(eq->getEnabled()) {
			eqEnabled = true;
			break;
		}
	}
	return eqEnabled;
}

void EqualizersController::show() {
	ui->bodePlot->enablePlotUpdate(true);
	QDialog::show();
	if(savedGeometry.isValid())
		setGeometry(savedGeometry);
}

void EqualizersController::onImportAction() {
	QString fileName = QFileDialog::getOpenFileName(
	    this, tr("Open EQ config"), "", tr("EqualizerAPO ParametricEq (*.txt);;All files (*)"));
	if(fileName.isEmpty())
		return;

	QFile inputFile(fileName);
	if(!inputFile.open(QIODevice::ReadOnly))
		return;

	// https://sourceforge.net/p/equalizerapo/wiki/Configuration%20reference/#configuration-file-format
	QTextStream in(&inputFile);

	QRegularExpression regexCommand("^Filter\\s+([0-9]+):(.*)");
	QRegularExpression regexType("^\\s*(ON|OFF)\\s+([A-Za-z]+)(.*)");
	QRegularExpression regexFreq("\\s+Fc\\s*([-+0-9.eE\u00A0]+)\\s*H\\s*z");
	QRegularExpression regexGain("\\s+Gain\\s*([-+0-9.eE]+)\\s*dB");
	QRegularExpression regexQ("\\s+Q\\s*([-+0-9.eE]+)");
	QRegularExpression regexBW("\\s+BW\\s+Oct\\s*([-+0-9.eE]+)");
	QRegularExpression regexSlope("^\\s*([-+0-9.eE]+)\\s*dB");
	QHash<QString, FilterType> filterTypeMap;

	filterTypeMap["None"] = FilterType::None;
	filterTypeMap["PK"] = FilterType::Peak;
	filterTypeMap["PEQ"] = FilterType::Peak;
	filterTypeMap["Modal"] = FilterType::Peak;
	filterTypeMap["LP"] = FilterType::LowPass;
	filterTypeMap["HP"] = FilterType::HighPass;
	filterTypeMap["LPQ"] = FilterType::LowPass;
	filterTypeMap["HPQ"] = FilterType::HighPass;
	filterTypeMap["BP"] = FilterType::BandPassConstantPeak;
	filterTypeMap["LS"] = FilterType::LowShelf;
	filterTypeMap["HS"] = FilterType::HighShelf;
	filterTypeMap["LSC"] = FilterType::LowShelf;
	filterTypeMap["HSC"] = FilterType::HighShelf;
	filterTypeMap["NO"] = FilterType::Notch;
	filterTypeMap["AP"] = FilterType::AllPass;

	int lineNumber = 0;
	while(!in.atEnd()) {
		lineNumber++;
		QString line = in.readLine();
		QRegularExpressionMatch match;

		if(!(match = regexCommand.match(line)).hasMatch())
			continue;

		long filterIndex = match.captured(1).toLong();
		if(filterIndex <= 0) {
			QMessageBox::warning(
			    this,
			    "Error in EQ config file",
			    QString("line %1: invalid filter index 0, must be >= 1: %2").arg(lineNumber).arg(line));
			continue;
		}
		QString parameters = match.captured(2);

		if(!(match = regexType.match(parameters)).hasMatch()) {
			QMessageBox::warning(this,
			                     "Error in EQ config file",
			                     QString("line %1: invalid filter type: %2").arg(lineNumber).arg(parameters));
			continue;
		}

		bool enabled = match.captured(1) == "ON";

		QString filterTypeString = match.captured(2);
		auto it = filterTypeMap.find(filterTypeString);

		if(it == filterTypeMap.end()) {
			QMessageBox::warning(this,
			                     "Error in EQ config file",
			                     QString("line %1: Unrecognized EQ type %2").arg(lineNumber).arg(filterTypeString));
			continue;
		}

		FilterType filterType = *it;
		parameters = match.captured(3);

		double fc = 0.0f;
		double gain = 0.0f;
		double bandwidthOrQOrS = 0;
		bool isBandwidthOrS = false;
		bool isCornerFreq = false;

		if((match = regexFreq.match(parameters)).hasMatch()) {
			fc = match.captured(1).toDouble();
		} else {
			QMessageBox::warning(this,
			                     "Error in EQ config file",
			                     QString("line %1: missing Fc argument: %2").arg(lineNumber).arg(parameters));
			continue;
		}

		if((match = regexGain.match(parameters)).hasMatch()) {
			gain = match.captured(1).toDouble();
		} else if(filterType == FilterType::Peak || filterType == FilterType::LowShelf ||
		          filterType == FilterType::HighShelf) {
			QMessageBox::warning(this,
			                     "Error in EQ config file",
			                     QString("line %1: missing Gain argument (required for Peak/Shelving filters): %2")
			                         .arg(lineNumber)
			                         .arg(parameters));
			continue;
		}

		if((match = regexQ.match(parameters)).hasMatch()) {
			bandwidthOrQOrS = match.captured(1).toDouble();
		}

		if((match = regexBW.match(parameters)).hasMatch()) {
			bandwidthOrQOrS = match.captured(1).toDouble();
			isBandwidthOrS = true;
		}

		if((match = regexSlope.match(parameters)).hasMatch()) {
			bandwidthOrQOrS = match.captured(1).toDouble();
			isBandwidthOrS = true;
		}

		if(bandwidthOrQOrS == 0) {
			if(filterType == FilterType::Peak || filterType == FilterType::AllPass) {
				QMessageBox::warning(
				    this,
				    "Error in EQ config file",
				    QString("line %1: missing Q or BW argument (required for Peak/AllPass filters): %2")
				        .arg(lineNumber)
				        .arg(parameters));
				continue;
			} else if(filterType == FilterType::LowPass || filterType == FilterType::HighPass) {
				bandwidthOrQOrS = M_SQRT1_2;
			} else if(filterType == FilterType::LowShelf || filterType == FilterType::HighShelf) {
				bandwidthOrQOrS = 0.9;
			} else if(filterType == FilterType::Notch) {
				bandwidthOrQOrS = 30.0;
			}
		} else if(filterType == FilterType::LowShelf || filterType == FilterType::HighShelf) {
			if(isBandwidthOrS) {
				// Maximum S is 1 for 12 dB
				bandwidthOrQOrS /= 12.0;
			}

			if(filterTypeString.back() != 'C')
				isCornerFreq = true;
		}

		if(isCornerFreq && (filterType == FilterType::LowShelf || filterType == FilterType::HighShelf)) {
			double s = bandwidthOrQOrS;
			if(!isBandwidthOrS)  // Q
			{
				double q = bandwidthOrQOrS;
				double a = pow(10.0, gain / 40.0);
				s = 1.0 / ((1.0 / (q * q) - 2.0) / (a + 1.0 / a) + 1.0);
			}

			// frequency adjustment for DCX2496
			double centerFreqFactor = pow(10.0, abs(gain) / 80.0 / s);
			if(filterType == FilterType::LowShelf)
				fc *= centerFreqFactor;
			else
				fc /= centerFreqFactor;
		}

		if(isBandwidthOrS) {
			if(filterType == FilterType::LowShelf || filterType == FilterType::HighShelf) {
				double A = pow(10.0, gain / 40.0);
				bandwidthOrQOrS = 1 / (sqrt((A + 1.0 / A) * (1.0 / bandwidthOrQOrS - 1.0) + 2.0));
			} else {
				double w0 = 2 * M_PI * fc / 48000;
				bandwidthOrQOrS = 1 / (2.0 * sinh(M_LN2 / 2.0 * bandwidthOrQOrS * w0 / sin(w0)));
			}
		}

		Equalizer* eqWidget = (Equalizer*) oscEqualizers.getWidget(filterIndex - 1);
		if(!eqWidget) {
			QMessageBox::warning(this,
			                     "Error in EQ config file",
			                     QString("line %1: EQ index does not exists: %2").arg(lineNumber).arg(filterIndex));
			continue;
		}

		eqWidget->setParameters(enabled, filterType, fc, gain, bandwidthOrQOrS);
	}
	inputFile.close();
}

void EqualizersController::onExportAction() {
	QString fileName = QFileDialog::getSaveFileName(
	    this, tr("Save EQ config"), "", tr("EqualizerAPO ParametricEq (*.txt);;All files (*)"));
	if(fileName.isEmpty())
		return;

	QFile outputFile(fileName);
	if(!outputFile.open(QIODevice::ReadWrite))
		return;

	QHash<int, QString> filterTypeMap;

	filterTypeMap[(int) FilterType::None] = "None";
	filterTypeMap[(int) FilterType::Peak] = "PK";
	filterTypeMap[(int) FilterType::LowPass] = "LP";
	filterTypeMap[(int) FilterType::HighPass] = "HP";
	filterTypeMap[(int) FilterType::BandPassConstantPeak] = "BP";
	filterTypeMap[(int) FilterType::LowShelf] = "LSC";
	filterTypeMap[(int) FilterType::HighShelf] = "HSC";
	filterTypeMap[(int) FilterType::Notch] = "NO";
	filterTypeMap[(int) FilterType::AllPass] = "AP";

	int filterIndex = 1;
	for(auto item : oscEqualizers.getWidgets()) {
		Equalizer* eq = (Equalizer*) item;

		bool enabled;
		FilterType filterType;
		double fc;
		double gain;
		double Q;

		eq->getParameters(enabled, filterType, fc, gain, Q);

		QString line = QString("Filter %1: %2 %3")
		                   .arg(filterIndex)
		                   .arg(enabled ? "ON" : "OFF")
		                   .arg(filterTypeMap[(int) filterType]);

		line += QString(" Fc %1 Hz").arg(fc);
		line += QString(" Gain %1 dB").arg(gain);
		line += QString(" Q %1").arg(Q);
		line += "\n";

		outputFile.write(line.toUtf8());

		filterIndex++;
	}
}

void EqualizersController::hideEvent(QHideEvent*) {
	qDebug("hide event");
	savedGeometry = geometry();
	ui->bodePlot->enablePlotUpdate(false);
}
