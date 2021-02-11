#include <opencv2/opencv.hpp>
#include <opencv2/highgui.hpp>
#include <opencv2/imgproc.hpp>
#include <algorithm>
#include <cstdlib>
#include <map>
#include "transform.hpp"
#include <string>
using namespace std;
using namespace cv;
const int NoOfChoice = 5;
const int NoOfQuestion = 6;
int i = 1; // ���° �л������� ��Ÿ���� ����
map<int, int> answerAll;

void OMR()
{
    map<int, int> standardAnswer; // ���� ����
    map<int, int> testerAnswer; // �л��� �亯
    standardAnswer.insert(make_pair(0, 0));
    standardAnswer.insert(make_pair(1, 1)); //Question 1: answer: B
    standardAnswer.insert(make_pair(2, 3)); //Question 2: answer: D
    standardAnswer.insert(make_pair(3, 0)); //Question 3: answer: A
    standardAnswer.insert(make_pair(4, 2)); //Question 4: answer: C
    standardAnswer.insert(make_pair(5, 1)); //Question 5: answer: B
    // make_pair(����1, ����2) ����1�� ����2�� �� pair�� �������

    Mat image, gray, blurred, edge;

    // �̹��������� ��Ȳ�� �°� �ҷ����� (string.append , to_string() ���)
    string filename = "omr3/omr_test_";
    filename.append(to_string(i));
    filename.append(".png");

    image = imread(filename, IMREAD_COLOR);
    // image = imread("omr_test_(?).png");


    if (image.empty()) // �̹����� ������
    {
        cout << i << "��° �л��� OMR �̹����� �����ϴ�." << endl;
        exit(1);
    }

    // �̹��� ó��
    cvtColor(image, gray, COLOR_BGR2GRAY); // ���ȭ
    GaussianBlur(gray, blurred, Size(5, 5), 0); // ����ó��
    Canny(blurred, edge, 75, 200); // ��輱


    Mat paper, warped, thresh;
    vector<vector<Point> > contours;
    vector<Vec4i> hierarchy;
    vector<Point> approxCurve, docCnt;

    // �ܰ��� �����ϴ� �ڵ�
    findContours(edge, contours, hierarchy, CV_RETR_EXTERNAL, CV_CHAIN_APPROX_SIMPLE, Point(0, 0));
    // CV_RETR_EXTERNAL : ���� �ʿܰ� �ܰ������� ����
    // CV_CHAIN_APPROX_SIMPLE : ����, ����, �밢 ������ �� ���� ����

    // ����
    sort(contours.begin(), contours.end(), compareContourAreas);


    // ������ �ܰ��� �ν�
    for (const auto& element : contours)
    {
        double perimeter = arcLength(element, true); // �ܰ��� ���� ��ȯ
        approxPolyDP(element, approxCurve, 0.05 * perimeter, true);

        if (approxCurve.size() == 4) // �簢�� ����(OMR ī��)
        {
            docCnt = approxCurve;
            break;
        }
    }


    //Step 2: OMR�������� �ν��ϱ� ���� ���� (OMR ī�带 �� 4���� �޾Ƽ� ������������ ��ġ�ų� ����.)

    four_point_transform(image, paper, docCnt); // docCnt�� ������ ������ 4��
    four_point_transform(gray, warped, docCnt);




    //Step 3: Extract the sets of bubbles (questionCnt)
    threshold(warped, thresh, 127, 255, CV_THRESH_BINARY_INV | CV_THRESH_OTSU); // ����ȭ
    //imshow("thresh", thresh);


    findContours(thresh, contours, hierarchy, CV_RETR_CCOMP, CV_CHAIN_APPROX_SIMPLE, Point(0, 0)); // �ܰ��� ����

    vector<vector<Point> > contours_poly(contours.size());
    vector<Rect> boundRect(contours.size());
    vector<vector<Point>> questionCnt;


    //Find document countour
    for (int i = 0; i < contours.size(); i++)
    {
        approxPolyDP(Mat(contours[i]), contours_poly[i], 0.1, true); // �� ����. (approxPolyDP�� ���ڰ� true �̸� ��, false�� �)
        int w = boundingRect(Mat(contours[i])).width;
        int h = boundingRect(Mat(contours[i])).height;
        double ar = (double)w / h;

        if (hierarchy[i][3] == -1) //No parent
            if (w >= 20 && h >= 20 && ar < 1.1 && ar > 0.9)
                questionCnt.push_back(contours_poly[i]);


    }

    //cout << "\nquestionCnt " << questionCnt.size() << "\n";

    //Step 4: ������ �亯���� ������.
    sort_contour(questionCnt, 0, (int)questionCnt.size(), string("top-to-bottom"));

    for (int i = 0; i < questionCnt.size(); i = i + NoOfChoice)
    {
        sort_contour(questionCnt, i, i + 5, string("left-to-right"));
    }


    //Step 5: Determine the marked answer
    for (int i = 0; i < questionCnt.size();)
    {
        int maxPixel = 0;
        int answerKey = 0;

        if (i == 0) // id�� �ν�
        {
            for (int j = 0; j < NoOfChoice; ++i, ++j) // �̹������� �� �ȼ� ���� ���� ���� �����Ѵ�.
            {
                Mat mask = Mat::zeros(thresh.size(), CV_8U);
                drawContours(mask, questionCnt, i, 255, CV_FILLED, 8, hierarchy, 0, Point());
                bitwise_and(mask, thresh, mask);

                if (countNonZero(mask) > 500) // �迭 ��� ���� 0(������)�� �ƴ� �ȼ��� ���� ��ȯ
                {
                    drawContours(paper, questionCnt, j, Scalar(255, 0, 0), 2, 8, hierarchy, 0, Point()); //�ν��� �� �Ķ������� ǥ��
                    answerKey += pow(2, 4 - j); // 2���� �� �հ�
                }
            }
        }

        else { // ���� ���
            for (int j = 0; j < NoOfChoice; ++i, ++j)
            {
                Mat mask = Mat::zeros(thresh.size(), CV_8U);
                drawContours(mask, questionCnt, i, 255, CV_FILLED, 8, hierarchy, 0, Point());
                bitwise_and(mask, thresh, mask);
                if (countNonZero(mask) > maxPixel)
                {
                    maxPixel = countNonZero(mask); // ��� mask���� 0 �� �ƴ� ����� ������ ��ȯ��.
                    answerKey = j; // answerKey : �׽��͸���, ��¥ ����
                }

            }
        }


        testerAnswer.insert(make_pair(i / NoOfChoice - 1, answerKey));


        if (i / NoOfChoice != 1) // id�� ����
        {
            cout << "Question: " << i / NoOfChoice - 1 << " Tester's Answer: " << answerKey << "\n";
        }
    }


    //Step 6: OMR ������ �´��� üũ�ϴ� �ڵ�
    map<int, int>::const_iterator itStandardAnswer;
    map<int, int>::const_iterator itTesterAnswer;

    itStandardAnswer = standardAnswer.begin();
    itTesterAnswer = testerAnswer.begin();

    int correct = 0;
    int currentQuestion = 0;
    int id;

    while (itStandardAnswer != standardAnswer.end())
    {
        if (itStandardAnswer == standardAnswer.begin()) //id ���� �Ķ������� ǥ��
        {
            //drawContours(paper, questionCnt, (currentQuestion * NoOfChoice) + itTesterAnswer->second, Scalar(255, 0, 0), 2, 8, hierarchy, 0, Point());

            id = itTesterAnswer->second; // id���� ���⿡ �ο��ϴ°ǵ�, �� �ν��� �ϳ��ϳ� �ϰԲ� ��� �ϴ��� �����ؼ� �ڵ��ϱ�!

            ++currentQuestion;
            ++itTesterAnswer;
            ++itStandardAnswer;

            continue;
        }

        if (itStandardAnswer->second == itTesterAnswer->second)
        {
            ++correct;
            //Circle in GREEN
            drawContours(paper, questionCnt, (currentQuestion * NoOfChoice) + itStandardAnswer->second, Scalar(0, 255, 0), 2, 8, hierarchy, 0, Point());
        }
        else //Circle in RED
        {
            drawContours(paper, questionCnt, (currentQuestion * NoOfChoice) + itStandardAnswer->second, Scalar(0, 0, 255), 2, 8, hierarchy, 0, Point());
        }

        ++currentQuestion;
        ++itTesterAnswer;
        ++itStandardAnswer;
    }

    //Step 7: ���� ǥ���ϴ� �ڵ�
    double score = ((double)correct / (NoOfQuestion - 1) * 100); // id�� �����ϰ� ���

    Scalar color;
    if (score >= 60.0)
        color = Scalar(0, 255, 0);
    else
        color = Scalar(0, 0, 255);

    putText(paper, to_string((int)score) + "%", Point(20, 30), FONT_HERSHEY_SIMPLEX, 0.9, color, 2);

    cout << "�л�" << id << "�� ����" << endl;
    cout << "�� ������ �� : " << currentQuestion-1 << endl;
    cout << "���� ������ �� : " << correct << "\n";
    cout << "������ " << score << "�� �Դϴ�." << "" << endl;
    cout << endl;
    imshow("Marked Paper", paper);
    waitKey();
    answerAll.insert(make_pair(id, score)); //id�� �߰�
    i++;

}

bool cmp(const pair<int, int>& a, const pair<int, int>& b) { //vec ������ ���� �Լ�
    if (a.second == b.second) return a.first < b.first;
    return a.second > b.second;
}



int main()
{

    int n; // �л��� �� (OMR ī�� ��)
    cout << "�л��� ���� �Է����ּ��� : ";
    cin >> n;

    for (int i = 0; i < n; i++) {
        OMR();
    }


    double sum = 0;
    double average = 0;
    map<int, int>::const_iterator itAnswerAll;
    itAnswerAll = answerAll.begin();

    cout << "===========����============" << endl;

    while (itAnswerAll != answerAll.end())
    {
        cout << "�л�" << itAnswerAll->first << "�� ������ " << itAnswerAll->second << "�� �Դϴ�." << endl;
        sum += itAnswerAll->second;
        ++itAnswerAll;
    }

    average = sum / answerAll.size();

    cout << "\n�հ�� " << sum << "�� �Դϴ�." << endl;
    cout << "��� ������ " << average << "�� �Դϴ�." << endl;

    vector<pair<int, int>> vec(answerAll.begin(), answerAll.end()); // ��� ������ ���� ���ͷ� ��ȯ
    sort(vec.begin(), vec.end(), cmp);

    cout << "\n===========���===========" << endl;

    for (auto num : vec) {
        cout << "�л�" << num.first << " " << num.second << "��" << endl;
    }

    return 0;
}