#include"GetCandidate.h"

std::vector<Candidate> GetCandidate::run(cv::Mat& Image)
{
	oriBgrImage_8UC3 = Image;
	cv::cvtColor(oriBgrImage_8UC3,gray_source_image,CV_BGR2GRAY);
	/*string filename = "./resource/CharacterClassifier";
	FileStorage fs;
    fs.open(filename, FileStorage::READ);
	if(fs.isOpened()){
		cout<<"okay"<<endl;
	} else {
		cout<<"no"<<endl;
	}
	//CharacterClassifier.load("./resource/CharacterClassifier");*/
	featureExtract();
	swtprocess();
	Filter();
	return candidateStore;
}

void GetCandidate::featureExtract()
{
	strVectorStore.clear();
	mser = MSER(2, 20, 2000, 0.25, 0.2, 200, 1.01, 0.003, 5);
	mser(this->gray_source_image,this->strVectorStore);
	
	/*Mat plot;
	plot.create(this->oriBgrImage_8UC3.size(), this->oriBgrImage_8UC3.type());
	plot = Scalar::all(255);
	for(int i =  0; i < this->strVectorStore.size(); i++){
		for (int j = 0; j < this->strVectorStore[i].size(); j++){
			plot.at<Vec3b>(i, j) = this->oriBgrImage_8UC3.at<Vec3b>(i, j);
		}
	}
	//imshow("mser", plot);*/
}

void GetCandidate::swtprocess()
{
	swt.initialize(this->gray_source_image,this->strVectorStore); 
}

void GetCandidate::ExtractCCfeatures()
{
	ccStore.clear();
	int Num = strVectorStore.size();
	Candidate cd;
	for(int i= 0; i < Num ;i++)
	{
		cd.pointsNum = strVectorStore[i].size();
		cd.max_i = strVectorStore[i][0].y;
		cd.min_i = strVectorStore[i][0].y;
		cd.max_j = strVectorStore[i][0].x; 
		cd.min_j = strVectorStore[i][0].x; 
		float temp = 0;
		float tempr = 0;
		float tempg = 0;
		float tempb = 0;
		for(int j = 1;j < cd.pointsNum;j++)
		{
			if(strVectorStore[i][j].y > cd.max_i)
				cd.max_i = strVectorStore[i][j].y;
			else if(strVectorStore[i][j].y < cd.min_i)
				cd.min_i = strVectorStore[i][j].y;

			if(strVectorStore[i][j].x > cd.max_j)
				cd.max_j = strVectorStore[i][j].x;
			else if(strVectorStore[i][j].x < cd.min_j)
				cd.min_j = strVectorStore[i][j].x;
			temp  +=  this->gray_source_image.at<unsigned char>(strVectorStore[i][j].y, strVectorStore[i][j].x);
			tempr +=  this->oriBgrImage_8UC3.at<cv::Vec3b>(strVectorStore[i][j].y, strVectorStore[i][j].x)[0];
			tempg +=  this->oriBgrImage_8UC3.at<cv::Vec3b>(strVectorStore[i][j].y, strVectorStore[i][j].x)[1];
			tempb +=  this->oriBgrImage_8UC3.at<cv::Vec3b>(strVectorStore[i][j].y, strVectorStore[i][j].x)[2];
		}
		cd.middle_i = (cd.max_i + cd.min_i)/2;
		cd.middle_j = (cd.max_j + cd.min_j)/2;
		cd.avergecolor = temp / cd.pointsNum;
		cd.averarer = tempr / cd.pointsNum;
		cd.averareg = tempg / cd.pointsNum;
		cd.averareb = tempb / cd.pointsNum;
		cd.index = i;
		cd.alive = true;
		cd.high = cd.max_i - cd.min_i;
		cd.width = cd.max_j - cd.min_j;
		swt.getStrokeWidth(strVectorStore[i],cd);
	    float var = 0;
		for(int j = 0; j< cd.pointsNum;j++)
			var += (gray_source_image.at<unsigned char>(strVectorStore[i][j].y, strVectorStore[i][j].x) - cd.avergecolor)*(gray_source_image.at<unsigned char>(strVectorStore[i][j].y, strVectorStore[i][j].x) - cd.avergecolor);
		cd.colorVariance = var/cd.pointsNum;
		float sum = 0;
		for(int i = 0; i < cd.pointsNum; i++){
			sum += cd.strokeWidh;
		}
		cd.StrokeMean = sum / cd.pointsNum;
		cd.highWidthRatio = double(cd.max_i - cd.min_i)/(cd.max_j - cd.min_j);
		float area = (cd.max_i - cd.min_i)*(cd.max_j - cd.min_j);
		cd.grayRatio = float(cd.pointsNum)/(area + 1);
		ccStore.push_back(cd);
	}
}

void GetCandidate::Filter()
{
	ExtractCCfeatures();
	candidateStore.clear();
	for(int i = 0; i< ccStore.size();i++)
	{
		Candidate cd =  ccStore[i];
		cv::Mat rtVector = cv::Mat(1,14,CV_32FC1);
		rtVector.at<float>(0,0) = cd.pointsNum;
		rtVector.at<float>(0,1) = cd.high;
		rtVector.at<float>(0,2) = cd.width;
		rtVector.at<float>(0,3) = cd.high/this->oriBgrImage_8UC3.rows;
		rtVector.at<float>(0,4) = cd.width/this->oriBgrImage_8UC3.cols;
		rtVector.at<float>(0,5) = cd.high/(cd.width + 1);
		rtVector.at<float>(0,6) = cd.strokeVariance;
		rtVector.at<float>(0,7) = cd.strokeWidh;
		rtVector.at<float>(0,8) = cd.strokeWidthRatio;
		rtVector.at<float>(0,9) = 0;
		rtVector.at<float>(0,10) = cd.avergecolor;
		rtVector.at<float>(0,11) = cd.colorVariance;
		rtVector.at<float>(0,12) = abs(cd.middle_i - cd.high/2);
		rtVector.at<float>(0,13) = abs(cd.middle_j - cd.width/2);
		
		Size sz = this->oriBgrImage_8UC3.size();
		double w = sz.width, h = sz.height;
		//(CharacterClassifier.predict_prob(rtVector) > 0.0) &&  ))&& ((cd.high*cd.width) > (w*h/5000.0)))
		//cout<<"variance "<<sqrt(cd.strokeVariance) << " Mean "<<cd.StrokeMean<<endl;
		if((cd.high*cd.width) < (w*h/200.0) && ((cd.high*cd.width) > (w*h/20000.0))) {
			//if(sqrt(cd.strokeVariance) / cd.StrokeMean < 1.2){
			if(cd.highWidthRatio < 3 && cd.highWidthRatio > 0.33){
				candidateStore.push_back(cd);
			}
		}
		/*if((cd.strokeVariance / cd.StrokeMean) < 35){
			candidateStore.push_back(cd);
		}*/
	}
}

