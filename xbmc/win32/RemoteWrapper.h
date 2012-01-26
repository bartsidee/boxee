#pragma once

class IRemoteStrategy
{
public:
  virtual bool Initialize() = 0;
  virtual void Reset() = 0;
  virtual void Update() = 0;
  virtual WORD GetButton() = 0;
  virtual bool IsHolding() = 0;
  virtual void Disconnect() {}
};

class CIRSSRemoteStrategy : public IRemoteStrategy
{
public:
   
  virtual bool Initialize();
  virtual void Reset();
  virtual void Update();
  virtual WORD GetButton();
  virtual bool IsHolding();
};

class CBoxeeRemoteStrategy : public IRemoteStrategy
{
public:

  virtual bool Initialize();
  virtual void Reset();
  virtual void Update();
  virtual WORD GetButton();
  virtual bool IsHolding();
};

class CRemoteWrapper 
{
public:
	
	CRemoteWrapper();
	~CRemoteWrapper();

	void SelectStrategy(bool bPreferIRSS);

	virtual bool Initialize() {
		return m_pRemoteStrategy->Initialize();
	}

	virtual void Reset() {
		m_pRemoteStrategy->Reset();
	}

	virtual void Update() {
		m_pRemoteStrategy->Update();
	}

	virtual WORD GetButton() {
		return m_pRemoteStrategy->GetButton();
	}

	virtual bool IsHolding() {
		return m_pRemoteStrategy->IsHolding();
	}

	virtual void Disconnect() {
		return m_pRemoteStrategy->Disconnect();
	}

    
private:
	IRemoteStrategy* m_pRemoteStrategy;
	HINSTANCE m_instance;
};

extern CRemoteWrapper g_RemoteControl;
