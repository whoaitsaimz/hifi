//
//  Application.h
//  interface/src
//
//  Created by Andrzej Kapolka on 5/10/13.
//  Copyright 2013 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_Application_h
#define hifi_Application_h

#include <gpu/GPUConfig.h>

#include <QApplication>
#include <QHash>
#include <QImage>
#include <QPointer>
#include <QSet>
#include <QStringList>
#include <QUndoStack>

#include <AbstractScriptingServicesInterface.h>
#include <AbstractViewStateInterface.h>
#include <EntityEditPacketSender.h>
#include <EntityTreeRenderer.h>
#include <GeometryCache.h>
#include <NetworkPacket.h>
#include <NodeList.h>
#include <OctreeQuery.h>
#include <PacketHeaders.h>
#include <PhysicsEngine.h>
#include <ScriptEngine.h>
#include <StDev.h>
#include <TextureCache.h>
#include <ViewFrustum.h>

#include "AudioClient.h"
#include "Bookmarks.h"
#include "Camera.h"
#include "DatagramProcessor.h"
#include "Environment.h"
#include "FileLogger.h"
#include "GLCanvas.h"
#include "Menu.h"
#include "PacketHeaders.h"
#include "Physics.h"
#include "Stars.h"
#include "avatar/Avatar.h"
#include "avatar/MyAvatar.h"
#include "devices/SixenseManager.h"
#include "scripting/ControllerScriptingInterface.h"
#include "scripting/WebWindowClass.h"
#include "ui/BandwidthDialog.h"
#include "ui/HMDToolsDialog.h"
#include "ui/ModelsBrowser.h"
#include "ui/NodeBounds.h"
#include "ui/OctreeStatsDialog.h"
#include "ui/RearMirrorTools.h"
#include "ui/SnapshotShareDialog.h"
#include "ui/LodToolsDialog.h"
#include "ui/LogDialog.h"
#include "ui/UpdateDialog.h"
#include "ui/overlays/Overlays.h"
#include "ui/ApplicationOverlay.h"
#include "ui/RunningScriptsWidget.h"
#include "ui/ToolWindow.h"
#include "octree/OctreeFade.h"
#include "octree/OctreePacketProcessor.h"
#include "UndoStackScriptingInterface.h"


class QGLWidget;
class QKeyEvent;
class QMouseEvent;
class QSystemTrayIcon;
class QTouchEvent;
class QWheelEvent;

class FaceTracker;
class MainWindow;
class Node;
class ProgramObject;
class ScriptEngine;

static const float NODE_ADDED_RED   = 0.0f;
static const float NODE_ADDED_GREEN = 1.0f;
static const float NODE_ADDED_BLUE  = 0.0f;
static const float NODE_KILLED_RED   = 1.0f;
static const float NODE_KILLED_GREEN = 0.0f;
static const float NODE_KILLED_BLUE  = 0.0f;

static const QString SNAPSHOT_EXTENSION  = ".jpg";
static const QString SVO_EXTENSION  = ".svo";
static const QString SVO_JSON_EXTENSION  = ".svo.json";
static const QString JS_EXTENSION  = ".js";
static const QString FST_EXTENSION  = ".fst";

static const float BILLBOARD_FIELD_OF_VIEW = 30.0f; // degrees
static const float BILLBOARD_DISTANCE = 5.56f;       // meters

static const int MIRROR_VIEW_TOP_PADDING = 5;
static const int MIRROR_VIEW_LEFT_PADDING = 10;
static const int MIRROR_VIEW_WIDTH = 265;
static const int MIRROR_VIEW_HEIGHT = 215;
static const float MIRROR_FULLSCREEN_DISTANCE = 0.389f;
static const float MIRROR_REARVIEW_DISTANCE = 0.722f;
static const float MIRROR_REARVIEW_BODY_DISTANCE = 2.56f;
static const float MIRROR_FIELD_OF_VIEW = 30.0f;

static const quint64 TOO_LONG_SINCE_LAST_SEND_DOWNSTREAM_AUDIO_STATS = 1 * USECS_PER_SECOND;

static const QString INFO_HELP_PATH = "html/interface-welcome.html";
static const QString INFO_EDIT_ENTITIES_PATH = "html/edit-commands.html";

#ifdef Q_OS_WIN
static const UINT UWM_IDENTIFY_INSTANCES = 
    RegisterWindowMessage("UWM_IDENTIFY_INSTANCES_{8AB82783-B74A-4258-955B-8188C22AA0D6}");
static const UINT UWM_SHOW_APPLICATION =
    RegisterWindowMessage("UWM_SHOW_APPLICATION_{71123FD6-3DA8-4DC1-9C27-8A12A6250CBA}");
#endif

class Application;
#if defined(qApp)
#undef qApp
#endif
#define qApp (static_cast<Application*>(QCoreApplication::instance()))

typedef bool (Application::* AcceptURLMethod)(const QString &);

class Application : public QApplication, public AbstractViewStateInterface, AbstractScriptingServicesInterface {
    Q_OBJECT

    friend class OctreePacketProcessor;
    friend class DatagramProcessor;

public:
    static Application* getInstance() { return qApp; } // TODO: replace fully by qApp
    static const glm::vec3& getPositionForPath() { return getInstance()->_myAvatar->getPosition(); }
    static glm::quat getOrientationForPath() { return getInstance()->_myAvatar->getOrientation(); }
    static glm::vec3 getPositionForAudio() { return getInstance()->_myAvatar->getHead()->getPosition(); }
    static glm::quat getOrientationForAudio() { return getInstance()->_myAvatar->getHead()->getFinalOrientationInWorldFrame(); }

    Application(int& argc, char** argv, QElapsedTimer &startup_time);
    ~Application();

    void loadScripts();
    QString getPreviousScriptLocation();
    void setPreviousScriptLocation(const QString& previousScriptLocation);
    void clearScriptsBeforeRunning();
    void initializeGL();
    void paintGL();
    void resizeGL(int width, int height);

    void keyPressEvent(QKeyEvent* event);
    void keyReleaseEvent(QKeyEvent* event);

    void focusOutEvent(QFocusEvent* event);
    void focusInEvent(QFocusEvent* event);

    void mouseMoveEvent(QMouseEvent* event, unsigned int deviceID = 0);
    void mousePressEvent(QMouseEvent* event, unsigned int deviceID = 0);
    void mouseReleaseEvent(QMouseEvent* event, unsigned int deviceID = 0);

    void touchBeginEvent(QTouchEvent* event);
    void touchEndEvent(QTouchEvent* event);
    void touchUpdateEvent(QTouchEvent* event);

    void wheelEvent(QWheelEvent* event);
    void dropEvent(QDropEvent *event);

    bool event(QEvent* event);
    bool eventFilter(QObject* object, QEvent* event);

    GLCanvas* getGLWidget() { return _glWidget; }
    bool isThrottleRendering() const { return _glWidget->isThrottleRendering(); }

    Camera* getCamera() { return &_myCamera; }
    // Represents the current view frustum of the avatar.  
    ViewFrustum* getViewFrustum();
    // Represents the view frustum of the current rendering pass, 
    // which might be different from the viewFrustum, i.e. shadowmap 
    // passes, mirror window passes, etc
    ViewFrustum* getDisplayViewFrustum();
    ViewFrustum* getShadowViewFrustum() { return &_shadowViewFrustum; }
    const OctreePacketProcessor& getOctreePacketProcessor() const { return _octreeProcessor; }
    EntityTreeRenderer* getEntities() { return &_entities; }
    Environment* getEnvironment() { return &_environment; }
    QUndoStack* getUndoStack() { return &_undoStack; }
    MainWindow* getWindow() { return _window; }
    OctreeQuery& getOctreeQuery() { return _octreeQuery; }
    EntityTree* getEntityClipboard() { return &_entityClipboard; }
    EntityTreeRenderer* getEntityClipboardRenderer() { return &_entityClipboardRenderer; }
    
    bool isMousePressed() const { return _mousePressed; }
    bool isMouseHidden() const { return !_cursorVisible; }
    const glm::vec3& getMouseRayOrigin() const { return _mouseRayOrigin; }
    const glm::vec3& getMouseRayDirection() const { return _mouseRayDirection; }
    bool mouseOnScreen() const;
    int getMouseX() const;
    int getMouseY() const;
    int getTrueMouseX() const { return _glWidget->mapFromGlobal(QCursor::pos()).x(); }
    int getTrueMouseY() const { return _glWidget->mapFromGlobal(QCursor::pos()).y(); }
    int getMouseDragStartedX() const;
    int getMouseDragStartedY() const;
    int getTrueMouseDragStartedX() const { return _mouseDragStartedX; }
    int getTrueMouseDragStartedY() const { return _mouseDragStartedY; }
    bool getLastMouseMoveWasSimulated() const { return _lastMouseMoveWasSimulated; }
    
    FaceTracker* getActiveFaceTracker();
    QSystemTrayIcon* getTrayIcon() { return _trayIcon; }
    ApplicationOverlay& getApplicationOverlay() { return _applicationOverlay; }
    Overlays& getOverlays() { return _overlays; }

    float getFps() const { return _fps; }
    const glm::vec3& getViewMatrixTranslation() const { return _viewMatrixTranslation; }
    void setViewMatrixTranslation(const glm::vec3& translation) { _viewMatrixTranslation = translation; }

    virtual const Transform& getViewTransform() const { return _viewTransform; }
    void setViewTransform(const Transform& view);
    
    float getFieldOfView() { return _fieldOfView.get(); }
    void setFieldOfView(float fov) { _fieldOfView.set(fov); }

    bool importSVOFromURL(const QString& urlString);

    NodeToOctreeSceneStats* getOcteeSceneStats() { return &_octreeServerSceneStats; }
    void lockOctreeSceneStats() { _octreeSceneStatsLock.lockForRead(); }
    void unlockOctreeSceneStats() { _octreeSceneStatsLock.unlock(); }

    ToolWindow* getToolWindow() { return _toolWindow ; }

    virtual AbstractControllerScriptingInterface* getControllerScriptingInterface() { return &_controllerScriptingInterface; }
    virtual void registerScriptEngineWithApplicationServices(ScriptEngine* scriptEngine);

    void resetProfile(const QString& username);

    void controlledBroadcastToNodes(const QByteArray& packet, const NodeSet& destinationNodeTypes);

    virtual void setupWorldLight();
    virtual bool shouldRenderMesh(float largestDimension, float distanceToCamera);

    QImage renderAvatarBillboard();

    void displaySide(Camera& whichCamera, bool selfAvatarOnly = false, RenderArgs::RenderSide renderSide = RenderArgs::MONO);

    /// Stores the current modelview matrix as the untranslated view matrix to use for transforms and the supplied vector as
    /// the view matrix translation.
    void updateUntranslatedViewMatrix(const glm::vec3& viewMatrixTranslation = glm::vec3());

    const glm::mat4& getUntranslatedViewMatrix() const { return _untranslatedViewMatrix; }

    /// Loads a view matrix that incorporates the specified model translation without the precision issues that can
    /// result from matrix multiplication at high translation magnitudes.
    void loadTranslatedViewMatrix(const glm::vec3& translation);

    void getModelViewMatrix(glm::dmat4* modelViewMatrix);
    void getProjectionMatrix(glm::dmat4* projectionMatrix);

    virtual const glm::vec3& getShadowDistances() const { return _shadowDistances; }

    /// Computes the off-axis frustum parameters for the view frustum, taking mirroring into account.
    virtual void computeOffAxisFrustum(float& left, float& right, float& bottom, float& top, float& nearVal,
        float& farVal, glm::vec4& nearClipPlane, glm::vec4& farClipPlane) const;

    virtual ViewFrustum* getCurrentViewFrustum() { return getDisplayViewFrustum(); }
    virtual bool getShadowsEnabled();
    virtual bool getCascadeShadowsEnabled();
    virtual QThread* getMainThread() { return thread(); }
    virtual float getSizeScale() const;
    virtual int getBoundaryLevelAdjust() const;
    virtual PickRay computePickRay(float x, float y);
    virtual const glm::vec3& getAvatarPosition() const { return _myAvatar->getPosition(); }

    NodeBounds& getNodeBoundsDisplay()  { return _nodeBoundsDisplay; }

    FileLogger* getLogger() { return _logger; }

    glm::vec2 getViewportDimensions() const { return glm::vec2(_glWidget->getDeviceWidth(),
                                                               _glWidget->getDeviceHeight()); }
    NodeToJurisdictionMap& getEntityServerJurisdictions() { return _entityServerJurisdictions; }

    void skipVersion(QString latestVersion);

    QStringList getRunningScripts() { return _scriptEnginesHash.keys(); }
    ScriptEngine* getScriptEngine(QString scriptHash) { return _scriptEnginesHash.contains(scriptHash) ? _scriptEnginesHash[scriptHash] : NULL; }
    
    bool isLookingAtMyAvatar(Avatar* avatar);

    float getRenderResolutionScale() const;
    int getRenderAmbientLight() const;

    unsigned int getRenderTargetFramerate() const;
    bool isVSyncOn() const;
    bool isVSyncEditable() const;
    bool isAboutToQuit() const { return _aboutToQuit; }

    // the isHMDmode is true whenever we use the interface from an HMD and not a standard flat display
    // rendering of several elements depend on that
    // TODO: carry that information on the Camera as a setting
    bool isHMDMode() const;
    
    QRect getDesirableApplicationGeometry();
    RunningScriptsWidget* getRunningScriptsWidget() { return _runningScriptsWidget; }

    Bookmarks* getBookmarks() const { return _bookmarks; }
    
    QString getScriptsLocation();
    void setScriptsLocation(const QString& scriptsLocation);
    
    void initializeAcceptedFiles();
    bool canAcceptURL(const QString& url);
    bool acceptURL(const QString& url);

signals:

    /// Fired when we're simulating; allows external parties to hook in.
    void simulating(float deltaTime);

    /// Fired when we're rendering in-world interface elements; allows external parties to hook in.
    void renderingInWorldInterface();

    /// Fired when we're rendering the overlay.
    void renderingOverlay();

    /// Fired when the import window is closed
    void importDone();
    
    void scriptLocationChanged(const QString& newPath);

    void svoImportRequested(const QString& url);

    void checkBackgroundDownloads();
    void domainConnectionRefused(const QString& reason);

    void faceURLChanged(const QString& newValue);
    void skeletonURLChanged(const QString& newValue);

public slots:
    void domainChanged(const QString& domainHostname);
    void updateWindowTitle();
    void nodeAdded(SharedNodePointer node);
    void nodeKilled(SharedNodePointer node);
    void packetSent(quint64 length);

    QVector<EntityItemID> pasteEntities(float x, float y, float z);
    bool exportEntities(const QString& filename, const QVector<EntityItemID>& entityIDs);
    bool exportEntities(const QString& filename, float x, float y, float z, float scale);
    bool importEntities(const QString& url);

    void setLowVelocityFilter(bool lowVelocityFilter);
    void loadDialog();
    void loadScriptURLDialog();
    void toggleLogDialog();
    bool acceptSnapshot(const QString& urlString);
    bool askToSetAvatarUrl(const QString& url);
    bool askToLoadScript(const QString& scriptFilenameOrURL);
    ScriptEngine* loadScript(const QString& scriptFilename = QString(), bool isUserLoaded = true, 
        bool loadScriptFromEditor = false, bool activateMainWindow = false);
    void scriptFinished(const QString& scriptName);
    void stopAllScripts(bool restart = false);
    void stopScript(const QString& scriptName);
    void reloadAllScripts();
    void loadDefaultScripts();
    void toggleRunningScriptsWidget();
    void saveScripts();
    void showFriendsWindow();
    void friendsWindowClosed();

    void packageModel();
    
    void openUrl(const QUrl& url);

    void updateMyAvatarTransform();
    
    void domainSettingsReceived(const QJsonObject& domainSettingsObject);

    void setVSyncEnabled();

    void resetSensors();
    void aboutApp();
    void showEditEntitiesHelp();
    
    void loadSettings();
    void saveSettings();

    void notifyPacketVersionMismatch();

    void setActiveFaceTracker();

    void domainConnectionDenied(const QString& reason);

private slots:
    void clearDomainOctreeDetails();
    void checkFPS();
    void idle();
    void aboutToQuit();
    
    void handleScriptEngineLoaded(const QString& scriptFilename);
    void handleScriptLoadError(const QString& scriptFilename);

    void connectedToDomain(const QString& hostname);

    friend class HMDToolsDialog;
    void setFullscreen(bool fullscreen);
    void setEnable3DTVMode(bool enable3DTVMode);
    void setEnableVRMode(bool enableVRMode);
    void cameraMenuChanged();

    glm::vec2 getScaledScreenPoint(glm::vec2 projectedPoint);

    void closeMirrorView();
    void restoreMirrorView();
    void shrinkMirrorView();

    void parseVersionXml();

    void manageRunningScriptsWidgetVisibility(bool shown);
    
    void runTests();
    
    void audioMuteToggled();

    void setCursorVisible(bool visible);

private:
    void resetCamerasOnResizeGL(Camera& camera, int width, int height);
    void updateProjectionMatrix();
    void updateProjectionMatrix(Camera& camera, bool updateViewFrustum = true);

    void updateCursorVisibility();

    void sendPingPackets();

    void initDisplay();
    void init();
    
    void cleanupBeforeQuit();

    void update(float deltaTime);

    // Various helper functions called during update()
    void updateLOD();
    void updateMouseRay();
    void updateMyAvatarLookAtPosition();
    void updateThreads(float deltaTime);
    void updateCamera(float deltaTime);
    void updateDialogs(float deltaTime);
    void updateCursor(float deltaTime);

    Avatar* findLookatTargetAvatar(glm::vec3& eyePosition, QUuid &nodeUUID);

    void renderLookatIndicator(glm::vec3 pointOfInterest);

    void queryOctree(NodeType_t serverType, PacketType packetType, NodeToJurisdictionMap& jurisdictions);
    void loadViewFrustum(Camera& camera, ViewFrustum& viewFrustum);

    glm::vec3 getSunDirection();

    void updateShadowMap();
    void renderRearViewMirror(const QRect& region, bool billboard = false);
    void setMenuShortcutsEnabled(bool enabled);

    static void attachNewHeadToNode(Node *newNode);
    static void* networkReceive(void* args); // network receive thread

    int sendNackPackets();

    bool _dependencyManagerIsSetup;
    MainWindow* _window;

    ToolWindow* _toolWindow;
    WebWindowClass* _friendsWindow;
    
    DatagramProcessor* _datagramProcessor;

    QUndoStack _undoStack;
    UndoStackScriptingInterface _undoStackScriptingInterface;

    glm::vec3 _gravity;

    // Frame Rate Measurement

    int _frameCount;
    float _fps;
    QElapsedTimer _applicationStartupTime;
    QElapsedTimer _timerStart;
    QElapsedTimer _lastTimeUpdated;
    bool _justStarted;
    Stars _stars;

    PhysicsEngine _physicsEngine;

    EntityTreeRenderer _entities;
    EntityTreeRenderer _entityClipboardRenderer;
    EntityTree _entityClipboard;

    ViewFrustum _viewFrustum; // current state of view frustum, perspective, orientation, etc.
    ViewFrustum _lastQueriedViewFrustum; /// last view frustum used to query octree servers (voxels)
    ViewFrustum _displayViewFrustum;
    ViewFrustum _shadowViewFrustum;
    quint64 _lastQueriedTime;

    float _trailingAudioLoudness;

    OctreeQuery _octreeQuery; // NodeData derived class for querying octee cells from octree servers

    MyAvatar* _myAvatar;            // TODO: move this and relevant code to AvatarManager (or MyAvatar as the case may be)

    Camera _myCamera;                  // My view onto the world
    Camera _mirrorCamera;              // Cammera for mirror view
    QRect _mirrorViewRect;
    RearMirrorTools* _rearMirrorTools;
    
    Setting::Handle<bool> _firstRun;
    Setting::Handle<QString> _previousScriptLocation;
    Setting::Handle<QString> _scriptsLocationHandle;
    Setting::Handle<float> _fieldOfView;

    Transform _viewTransform;
    glm::mat4 _untranslatedViewMatrix;
    glm::vec3 _viewMatrixTranslation;
    glm::mat4 _projectionMatrix;

    float _scaleMirror;
    float _rotateMirror;
    float _raiseMirror;

    static const int CASCADED_SHADOW_MATRIX_COUNT = 4;
    glm::mat4 _shadowMatrices[CASCADED_SHADOW_MATRIX_COUNT];
    glm::vec3 _shadowDistances;

    Environment _environment;

    bool _cursorVisible;
    int _mouseDragStartedX;
    int _mouseDragStartedY;
    quint64 _lastMouseMove;
    bool _lastMouseMoveWasSimulated;

    glm::vec3 _mouseRayOrigin;
    glm::vec3 _mouseRayDirection;

    float _touchAvgX;
    float _touchAvgY;
    float _touchDragStartedAvgX;
    float _touchDragStartedAvgY;
    bool _isTouchPressed; //  true if multitouch has been pressed (clear when finished)

    bool _mousePressed; //  true if mouse has been pressed (clear when finished)

    QSet<int> _keysPressed;

    bool _enableProcessOctreeThread;

    OctreePacketProcessor _octreeProcessor;
    EntityEditPacketSender _entityEditSender;

    StDev _idleLoopStdev;
    float _idleLoopMeasuredJitter;

    int parseOctreeStats(const QByteArray& packet, const SharedNodePointer& sendingNode);
    void trackIncomingOctreePacket(const QByteArray& packet, const SharedNodePointer& sendingNode, bool wasStatsPacket);

    NodeToJurisdictionMap _entityServerJurisdictions;
    NodeToOctreeSceneStats _octreeServerSceneStats;
    QReadWriteLock _octreeSceneStatsLock;

    NodeBounds _nodeBoundsDisplay;

    std::vector<OctreeFade> _octreeFades;
    QReadWriteLock _octreeFadesLock;
    ControllerScriptingInterface _controllerScriptingInterface;
    QPointer<LogDialog> _logDialog;
    QPointer<SnapshotShareDialog> _snapshotShareDialog;

    FileLogger* _logger;

    void checkVersion();
    void displayUpdateDialog();
    bool shouldSkipVersion(QString latestVersion);
    void takeSnapshot();

    TouchEvent _lastTouchEvent;

    Overlays _overlays;
    ApplicationOverlay _applicationOverlay;

    RunningScriptsWidget* _runningScriptsWidget;
    QHash<QString, ScriptEngine*> _scriptEnginesHash;
    bool _runningScriptsWidgetWasVisible;
    QString _scriptsLocation;

    QSystemTrayIcon* _trayIcon;

    quint64 _lastNackTime;
    quint64 _lastSendDownstreamAudioStats;

    bool _isVSyncOn;
    
    bool _aboutToQuit;

    Bookmarks* _bookmarks;

    bool _notifiedPacketVersionMismatchThisDomain;
    
    QThread _settingsThread;
    QTimer _settingsTimer;
    
    GLCanvas* _glWidget = new GLCanvas(); // our GLCanvas has a couple extra features

    void checkSkeleton();

    QWidget* _fullscreenMenuWidget = new QWidget();
    int _menuBarHeight;
    
    QHash<QString, AcceptURLMethod> _acceptedExtensions;

    QList<QString> _domainConnectionRefusals;
};

#endif // hifi_Application_h
