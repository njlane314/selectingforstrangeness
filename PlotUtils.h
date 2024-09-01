#ifndef _plot_utils_h_
#define _plot_utils_h_

#include "TH1D.h"
#include "TH2D.h"
#include "TMatrixD.h"
#include "TCanvas.h"
#include "TPad.h"
#include "TLegend.h"
#include <vector>
#include <string>
#include <cmath>
#include <stdexcept>

namespace plot_utils {

    const double single_canvas_x = 800;
    const double single_canvas_y = 600; 
    const double single_pad_split = 0.85;
    const double single_xaxis_title_size = 0.05;
    const double single_yaxis_title_size = 0.05;
    const double single_xaxis_title_offset = 0.93;
    const double single_yaxis_title_offset = 1.06;
    const double single_xaxis_label_size = 0.045;
    const double single_yaxis_label_size = 0.045;
    const double single_text_label_size = 0.09; 

    const double matrix_xaxis_title_size = 0.05;
    const double matrix_yaxis_title_size = 0.05;
    const double matrix_xaxis_title_offset = 0.93;
    const double matrix_yaxis_title_offset = 1.02;
    const double matrix_xaxis_label_size = 0.045;
    const double matrix_yaxis_label_size = 0.045;
    const double matrix_zaxis_label_size = 0.045;

    const int good_line_colors[13] = {1, 2, 8, 4, 6, 38, 46, 43, 30, 9, 7, 14, 3};

    void draw_histogram(TH1D* hist, const std::string& title, const std::string& plotdir) {
        TCanvas* c = new TCanvas("c", "c", single_canvas_x, single_canvas_y);
        hist->SetTitle(title.c_str());
        hist->Draw("HIST");
        hist->GetXaxis()->SetTitleSize(single_xaxis_title_size);
        hist->GetYaxis()->SetTitleSize(single_yaxis_title_size);
        hist->GetXaxis()->SetTitleOffset(single_xaxis_title_offset);
        hist->GetYaxis()->SetTitleOffset(single_yaxis_title_offset);
        hist->GetXaxis()->SetLabelSize(single_xaxis_label_size);
        hist->GetYaxis()->SetLabelSize(single_yaxis_label_size);
        
        c->SaveAs((plotdir + "/" + title + ".png").c_str());
        c->SaveAs((plotdir + "/" + title + ".pdf").c_str());
        delete c;
    }

    void draw_matrix(TH2D* h, const std::string& title, const std::string& plotdir, bool uselabels = false, bool usetext = false) {
        h->SetContour(1000);
        h->GetXaxis()->SetTitleSize(matrix_xaxis_title_size);
        h->GetYaxis()->SetTitleSize(matrix_yaxis_title_size);
        h->GetXaxis()->SetTitleOffset(matrix_xaxis_title_offset);
        h->GetYaxis()->SetTitleOffset(matrix_yaxis_title_offset);
        h->GetXaxis()->SetLabelSize(matrix_xaxis_label_size);
        h->GetYaxis()->SetLabelSize(matrix_yaxis_label_size);
        h->GetZaxis()->SetLabelSize(matrix_zaxis_label_size);

        TCanvas* c = new TCanvas("c", "c", single_canvas_x, single_canvas_y);
        c->SetRightMargin(0.13);

        if (uselabels) {
            c->SetLeftMargin(0.15);
            h->GetXaxis()->SetLabelSize(matrix_zaxis_label_size);
            h->GetYaxis()->SetLabelSize(matrix_zaxis_label_size);
        }

        if (usetext) {
            const int sf = 3;
            c->SetRightMargin(0.05);
            h->Draw("col");
            h->SetMarkerSize(3);
            for (int i = 1; i < h->GetNbinsX() + 1; i++) {
                for (int j = 1; j < h->GetNbinsX() + 1; j++) {
                    double content = std::round(h->GetBinContent(i, j) * std::pow(10, sf)) / std::pow(10, sf);
                    TLatex* text = new TLatex(h->GetXaxis()->GetBinCenter(i), h->GetYaxis()->GetBinCenter(j), std::to_string(content).c_str());
                    text->SetTextAlign(22);
                    text->Draw();
                    text->SetTextSize(0.04);
                }
            }
        } else {
            h->Draw("colz");
        }

        h->SetStats(0);

        c->SaveAs((plotdir + "/" + title + ".png").c_str());
        c->SaveAs((plotdir + "/" + title + ".pdf").c_str());
        delete c;
    }

} 

#endif // _plot_utils_h_