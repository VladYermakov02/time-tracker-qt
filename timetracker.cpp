#include "timetracker.h"
#include "./ui_timetracker.h"

TimeTracker::TimeTracker(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::TimeTracker)
    , timer(new QTimer(this))
    , previousScreen(nullptr)
{
    ui->setupUi(this);

    // Creating a QScrollArea to host the contents
    QScrollArea *scrollArea = new QScrollArea(this);
    scrollArea->setWidgetResizable(true);
    QWidget *containerWidget = new QWidget;
    screensGridLayout = new QGridLayout(containerWidget);

    QToolBar *toolbar = new QToolBar(this);
    startEndAction = new QAction("Start", this);
    connect(startEndAction, &QAction::triggered, this, &TimeTracker::startEndButtonClicked);
    toolbar->addAction(startEndAction);

    QAction *refreshAction = new QAction("Refresh", this);
    connect(refreshAction, &QAction::triggered, this, &TimeTracker::updateGridView);
    toolbar->addAction(refreshAction);

    scrollArea->setWidget(containerWidget);
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->addWidget(toolbar);
    mainLayout->addWidget(scrollArea);

    connect(timer, &QTimer::timeout, this, &TimeTracker::takeScreenshot);
    connect(this, &TimeTracker::newItemAdded, this, &TimeTracker::updateGridView);

    timeTrackerDB = QSqlDatabase::addDatabase("QSQLITE");
    timeTrackerDB.setDatabaseName("H:/programming/IDEs/SQLiteStudio/MyDBs/timetracker/timetracker.sqlite");
    if (!timeTrackerDB.open())
    {
        QMessageBox::information(this, "Not connected", "Database is Not Connected");
        QSqlQuery query;
        query.exec("CREATE TABLE IF NOT EXISTS images ("
                   "id INTEGER PRIMARY KEY AUTOINCREMENT, "
                   "image BLOB, "
                   "similarity REAL,"
                   "hash_sum TEXT"
                   ")");
    }

    updateGridView();
}

TimeTracker::~TimeTracker()
{
    timeTrackerDB.close();
    delete ui;
    delete timer;
}

void TimeTracker::startEndButtonClicked()
{
    if (timer->isActive())
    {
        timer->stop();
        this->startEndAction->setText("Start");
    }
    else
    {
        timer->start(60000); // 60 seconds
        this->startEndAction->setText("Stop");
    }
}

QByteArray TimeTracker::pixmapToByteArray(const QPixmap &pixmap)
{
    QByteArray byteArray;
    QBuffer buffer(&byteArray);
    buffer.open(QIODevice::WriteOnly);
    pixmap.save(&buffer, "PNG");
    return byteArray;
}

qint64 TimeTracker::calculateHashSum(const QByteArray &byteArray)
{
    qint64 sum = 0;
    for (char byte : byteArray)
    {
        sum += byte;
    }
    return sum;
}

// MSE algorithm
double TimeTracker::calculateSimilarity(QByteArray &image1)
{
    if (image1.size() != this->previousScreen.size())
    {
        return 0.0; // Images have different sizes, so they can't be compared.
    }
    int totalPixels = image1.size();
    double sumSquaredDiff = 0.0;
    for (int i = 0; i < totalPixels; i++)
    {
        int pixelDiff = static_cast<int>(static_cast<unsigned char>(image1[i])) - static_cast<int>(static_cast<unsigned char>(this->previousScreen[i]));
        sumSquaredDiff += pixelDiff * pixelDiff;
    }
    double mse = sumSquaredDiff / totalPixels;
    // Calculating the similarity as a percentage (lower MSE means higher similarity)
    double similarity = 1.0 - (mse / 255.0);
    return similarity * 100.0; // Converting to a percentage
}

void TimeTracker::takeScreenshot()
{
    QScreen *screen = QGuiApplication::primaryScreen();
    QPixmap screenshot = screen->grabWindow(0);

    QByteArray imageByteArray = pixmapToByteArray(screenshot);
    qint64 hashSum = calculateHashSum(imageByteArray);

    if (previousScreen != nullptr)
    {
        setLastScreenFromDB();
        // Runing calculateSimilarity in a separate thread
        QFuture<double> future = QtConcurrent::run(this, &TimeTracker::calculateSimilarity, imageByteArray);
        // Lock mutex until calculateSimilarity is done
        similarityMutex.lock();
        double similarity = future.result();
        similarityMutex.unlock();
        uploadToDatabase(imageByteArray, hashSum, similarity);
        emit newItemAdded();
    }
    else
    {
        uploadToDatabase(imageByteArray, hashSum, 0.0);
        setLastScreenFromDB();
    }
}

void TimeTracker::uploadToDatabase(const QByteArray &imageByteArray, const qint64 hashSum, const double similarity)
{
    QSqlQuery query;
    query.prepare("INSERT INTO images (image, similarity, hash_sum) VALUES (:image, :similarity, :hash_sum)");
    query.bindValue(":image", imageByteArray);
    query.bindValue(":similarity", similarity);
    query.bindValue(":hash_sum", hashSum);
    if (!query.exec())
    {
        qDebug() << "Error: Failed to execute query - " << query.lastError().text();
        return;
    }
    qDebug() << "Data uploaded successfully";
}

void TimeTracker::clearGridLayout(QGridLayout *gridLayout) {
    QLayoutItem *child;
    while ((child = gridLayout->takeAt(0)) != nullptr) {
        QWidget *widget = child->widget();
        if (widget) {
            delete widget;
        }
        delete child;
    }
}

void TimeTracker::updateGridView()
{
    clearGridLayout(screensGridLayout);
    QSqlQuery query("SELECT image, similarity, hash_sum FROM images ORDER BY id DESC");

    int row = 0;
    while (query.next()) {
        QByteArray imageBytes = query.value(0).toByteArray();
        double similarity = query.value(1).toDouble();
        qint64 hashSum = query.value(2).toInt();

        QLabel *imageLabel = new QLabel;
        QLabel *numberLabel = new QLabel(QString::number(row + 1));
        QLabel *similarityLabel = new QLabel("Similarity with the previous screenshot: " + QString::number(similarity));
        QLabel *hashSumLabel = new QLabel("Hash Sum:" + QString::number(hashSum));

        screensGridLayout->addWidget(imageLabel, row, 0);
        screensGridLayout->addWidget(numberLabel, row, 1);
        screensGridLayout->addWidget(similarityLabel, row, 2);
        screensGridLayout->addWidget(hashSumLabel, row, 3);

        QPixmap pixmap;
        pixmap.loadFromData(imageBytes);
        pixmap = pixmap.scaled(1500, 1500, Qt::KeepAspectRatio);
        imageLabel->setPixmap(pixmap);

        row++;
    }
}

void TimeTracker::setLastScreenFromDB()
{
    QSqlQuery query("SELECT image FROM images ORDER BY id DESC LIMIT 1");

    if (query.next()) {
        this->previousScreen = query.value(0).toByteArray();
    }
}

//QString TimeTracker::calculateHash(const QByteArray &byteArray)
//{
//    QCryptographicHash hash(QCryptographicHash::Algorithm::Sha256);
//    hash.addData(byteArray);
//    return hash.result().toHex();
//}

//double calculateSimilaritySSIM(const QByteArray &image1, const QByteArray &image2)
//{
//    cv::Mat mat1 = cv::imdecode(cv::Mat(image1.toVector(), false), cv::IMREAD_UNCHANGED);
//    cv::Mat mat2 = cv::imdecode(cv::Mat(image2.toVector(), false), cv::IMREAD_UNCHANGED);
//    if (mat1.empty() || mat2.empty())
//    {
//        return 0.0; // Unable to decode images.
//    }
//    cv::Mat mat1_gray, mat2_gray;
//    cv::cvtColor(mat1, mat1_gray, cv::COLOR_BGR2GRAY);
//    cv::cvtColor(mat2, mat2_gray, cv::COLOR_BGR2GRAY);
//    double ssim = cv::quality::QualitySSIM::compute(mat1_gray, mat2_gray).val[0];
//    return ssim * 100.0; // Convert to a percentage (0-100%)
//}
