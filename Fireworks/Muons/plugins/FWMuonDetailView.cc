// -*- C++ -*-
//
// Package:     Muons
// Class  :     FWMuonDetailView
//
// Implementation:
//     <Notes on implementation>
//
// Original Author:
//         Created:  Sun Jan  6 23:57:00 EST 2008
// $Id: FWMuonDetailView.cc,v 1.7 2009/08/22 20:34:11 amraktad Exp $
//

// system include files
#include "Rtypes.h"
#include "TEveViewer.h"
#include "TEveScene.h"
#include "TGLViewer.h"
#include "TGFrame.h"
#include "TGLabel.h"

#include "TEveGeoNode.h"
#include "TEveStraightLineSet.h"
#include "TEveTrack.h"
#include "TEveTrackPropagator.h"

#include "Fireworks/Core/interface/FWDetailView.h"
#include "Fireworks/Core/interface/FWModelId.h"
#include "Fireworks/Core/interface/FWEventItem.h"
#include "Fireworks/Core/interface/DetIdToMatrix.h"

#include "DataFormats/MuonReco/interface/Muon.h"
#include "DataFormats/FWLite/interface/Handle.h"
#include "DataFormats/TrackReco/interface/Track.h"
#include "DataFormats/MuonReco/interface/Muon.h"
#include "DataFormats/MuonReco/interface/MuonFwd.h"
#include "DataFormats/MuonDetId/interface/MuonSubdetId.h"
#include "DataFormats/MuonReco/interface/MuonIsolation.h"

#include "Fireworks/Core/interface/BuilderUtils.h"


class FWMuonDetailView : public FWDetailView<reco::Muon> {

public:
   FWMuonDetailView();
   virtual ~FWMuonDetailView();

   virtual void build(const FWModelId &id, const reco::Muon*, TEveWindowSlot*);

protected:
   void setItem (const FWEventItem *iItem) {
      m_item = iItem;
   }
   void build (TEveElementList **product);

private:
   FWMuonDetailView(const FWMuonDetailView&); // stop default
   const FWMuonDetailView& operator=(const FWMuonDetailView&); // stop default

   // ---------- member data --------------------------------
   const FWEventItem* m_item;
};

//
// constructors and destructor
//
FWMuonDetailView::FWMuonDetailView()
{
}

FWMuonDetailView::~FWMuonDetailView()
{
}

//
// member functions
//
void FWMuonDetailView::build (const FWModelId &id, const reco::Muon* iMuon, TEveWindowSlot* slot)
{
   if(0 == iMuon) { return; }

   TEveScene* scene;
   TEveViewer* eviewer;
   TGVerticalFrame* ediFrame;
   FWDetailViewBase::makePackViewer(slot, ediFrame, eviewer, scene);

   /* Here we have the code imported from the Electron variant
    * Differences in implementation for proxy building
    *
    * Electrons implement both clusters and superclusters.  For muons, we want
    * something simpler in the ECAL: just the crystals hit.
    *
    * For muons, we also don't need to focus so much on the ECAL/tracker border, but
    * also want to look into the muon system.  This means drawing in the hit segments
    * in the muon system and the hits in the tracking system.
    *
    */
   m_item = id.item();

   const fwlite::Event *ev = m_item->getEvent();
   fwlite::Handle<EcalRecHitCollection> e_hits;
   fwlite::Handle<CaloTowerCollection> towers;
   // Trying to get the Ecal and Hcal recHits from the event.
   fwlite::Handle<TrackingRecHitCollection> t_hits;
   const EcalRecHitCollection* ehits(0);
   const TrackingRecHitCollection* thits(0);
   const CaloTowerCollection* ctowers(0);

   try {
      towers.getByLabel(*ev, "towerMaker");
      ctowers = towers.ptr();
   }
   catch (...)
   {
      std::cout <<"Expected Cal Towers." << std::endl;
   }

   try {
      e_hits.getByLabel(*ev, "EcalRecHit", "EcalRecHitsEB");
      ehits = e_hits.ptr();
   }
   catch (...)
   {
      std::cout <<"no ehits are ECAL rechits are available, show only crystal location" << std::endl;
   }
   try {
      t_hits.getByLabel(*ev, "trackingRecHit");
      thits = t_hits.ptr();
   }
   catch (...)
   {
      std::cout <<"no thits are tracker rechits are available, show only crystal location" << std::endl;
   }

   /* Here's where we begin to diverge significantly from the electron things.
    * For the electrons, we have a single TEveTrackPropagator, while for the
    * muons we have 3.  So we're yoinking from MuonsProxy3DBuilder what we need
    */

   TEveTrackPropagator* innerPropagator = new TEveTrackPropagator();
   TEveTrackPropagator* innerShortPropagator = new TEveTrackPropagator();
   TEveTrackPropagator* outerPropagator = new TEveTrackPropagator();
   //units are Telsa
   innerPropagator->SetMagField( -4.0);
   innerPropagator->SetMaxR( 350 );
   innerPropagator->SetMaxZ( 650 );
   innerShortPropagator->SetMagField( -4.0);
   innerShortPropagator->SetMaxR( 120 );
   innerShortPropagator->SetMaxZ( 300 );
   outerPropagator->SetMagField( 2.5);
   outerPropagator->SetMaxR( 750 );
   outerPropagator->SetMaxZ( 1100 );

   int index=0;
   TEveRecTrack innerRecTrack;
   TEveRecTrack outerRecTrack;   if (const reco::Muon *muon = iMuon) {

      std::stringstream s;
      s << "muon" << index;
      //in order to keep muonList having the same number of elements as 'muons' we will always
      // create a list even if it will get no children
      TEveElementList* muonList = new TEveElementList(s.str().c_str());
      scene->AddElement( muonList);

      // These are the for the Eve side of things

      innerRecTrack.fP = TEveVector( muon->p4().px(), muon->p4().py(), muon->p4().pz() );
      innerRecTrack.fV = TEveVector( muon->vertex().x(), muon->vertex().y(), muon->vertex().z() );
      innerRecTrack.fSign = muon->charge();

      // This is for the actual muon location at the outside of the tracker
      try {
         reco::TrackRef muTrack = muon->track();
         std::cout << "xyz of track outer point = " << muTrack->outerPosition() << std::endl;
      }
      catch(...) {
         std::cout << "outerPosition of trackRef unavailable, using final muon position instead" << std::endl;
      }

      TEveTrack* innerTrack = 0;
      if ( muon->numberOfMatches(reco::Muon::SegmentAndTrackArbitration) >= 2 )
         innerTrack = new TEveTrack( &innerRecTrack, innerPropagator );
      else
         innerTrack = new TEveTrack( &innerRecTrack, innerShortPropagator );

      innerTrack->SetMainColor( m_item->defaultDisplayProperties().color() );
      // For this display, we want to see only the "muons" that make it to the chambers
      if ( muon->numberOfMatches(reco::Muon::SegmentAndTrackArbitration) < 2 ) return;
      innerTrack->MakeTrack();
      muonList->AddElement(innerTrack);

      // get last two points of the innerTrack trajectory
      // NOTE: if RECO is available we can use the stand alone muon track
      //       inner most state as a starting point for the outter track

      Double_t vx2, vy2, vz2, vx1, vy1, vz1;
      innerTrack->GetPoint( innerTrack->GetLastPoint(),   vx2, vy2, vz2);
      innerTrack->GetPoint( innerTrack->GetLastPoint()-1, vx1, vy1, vz1);

      // So we now have the tracks, let's get the ECAL crystals of the hit

      Int_t nEtaEcal = 3;
      Int_t nPhiEcal = 3;
      // Int_t nPhiHcal = 2;

      // This is the original from the ElectronsProxySCBuilder.  Mine will be based on
      // the trackref.

      TEveElement* el = fw::getEcalCrystals(ehits, *m_item->getGeom(),
                                            (*muon).eta(), (*muon).phi(),
                                            nEtaEcal, nPhiEcal);
      if (el) scene->AddElement(el);


      std::cout << "Printing Muon related quantities" << std::endl;
      std::cout << "isEnergyValid(): " << (*muon).isEnergyValid() << std::endl;
      std::cout << "isCaloCompatibilityValid(): " << (*muon).isCaloCompatibilityValid() << std::endl;
      std::cout << "caloCompatibility(): " << (*muon).caloCompatibility() << std::endl;
      std::cout << "Energies along the way: " << std::endl;
      std::cout << "Energy in the ecal: " << (*muon).calEnergy().em << std::endl;
      std::cout << "Energy in the hcal: " << (*muon).calEnergy().had << std::endl;
      std::cout << "Energy in the outer hcal: " << (*muon).calEnergy().ho << std::endl;

      TEveTrack* outerTrack = 0;

      // second track only for barrel for now
      if ( fabs(vz2) < 650 ) {
         outerRecTrack.fV = TEveVector(vx2,vy2,vz2);
         // use muon momentum at IP as an estimate of its momentum at the solenoid
         // and last two points of the inner track to get direction.
         // NOTE: RECO can provide better estimate
         float scale = muon->p4().P()/sqrt( (vx2-vx1)*(vx2-vx1) + (vy2-vy1)*(vy2-vy1) + (vz2-vz1)*(vz2-vz1) );
         outerRecTrack.fP = TEveVector(scale*(vx2-vx1), scale*(vy2-vy1),scale*(vz2-vz1));
         outerRecTrack.fSign = innerRecTrack.fSign;
         outerTrack = new TEveTrack( &outerRecTrack, outerPropagator );
         outerTrack->SetMainColor( m_item->defaultDisplayProperties().color() );
         // std::cout << "\tpx " << outerRecTrack.fP.fX << " py " << outerRecTrack.fP.fY << " pz " << outerRecTrack.fP.fZ
         //  << " vx " << outerRecTrack.fV.fX << " vy " << outerRecTrack.fV.fY << " vz " << outerRecTrack.fV.fZ
         //  << " sign " << outerRecTrack.fSign << std::endl;
         muonList->AddElement( outerTrack );
      }

      // add muon segments
      const std::vector<reco::MuonChamberMatch>& matches = muon->matches();
      Double_t localTrajectoryPoint[3];
      Double_t globalTrajectoryPoint[3];
      //need to use auto_ptr since the segmentSet may not be passed to muonList
      std::auto_ptr<TEveStraightLineSet> segmentSet(new TEveStraightLineSet);
      segmentSet->SetLineWidth(4);
      std::vector<reco::MuonChamberMatch>::const_iterator chamber = matches.begin();
      for ( ; chamber != matches.end(); ++chamber )
      {
         // expected track position
         localTrajectoryPoint[0] = chamber->x;
         localTrajectoryPoint[1] = chamber->y;
         localTrajectoryPoint[2] = 0;

         DetId id = chamber->id;
         const TGeoHMatrix* matrix = m_item->getGeom()->getMatrix( chamber->id.rawId() );
         TEveGeoShape* shape = m_item->getGeom()->getShape( chamber->id.rawId() );
         if(0!=shape) {
            shape->SetMainTransparency(50);
            shape->SetMainColor(m_item->defaultDisplayProperties().color());
            muonList->AddElement(shape);
         }

         if ( matrix ) {
            // make muon segment 20 cm long along local z-axis
            matrix->LocalToMaster( localTrajectoryPoint, globalTrajectoryPoint );

            // add path marks to force outer propagator to follow the expected
            // track position
            if ( outerTrack ) {
               // ROOT can suck my motherfucking dick
               TEvePathMark mark( TEvePathMark::kDaughter );
               mark.fV = TEveVector( globalTrajectoryPoint[0], globalTrajectoryPoint[1], globalTrajectoryPoint[2] );
               outerTrack->AddPathMark( mark );
            }

            // std::cout << "\t " << " vx " << globalTrajectoryPoint[0] << " vy " << globalTrajectoryPoint[1] <<
            //  " vz " << globalTrajectoryPoint[2] <<  std::endl;

            // add segments
            for ( std::vector<reco::MuonSegmentMatch>::const_iterator segment = chamber->segmentMatches.begin();
                  segment != chamber->segmentMatches.end(); ++segment )
            {
               Double_t localSegmentInnerPoint[3];
               Double_t localSegmentOuterPoint[3];
               Double_t globalSegmentInnerPoint[3];
               Double_t globalSegmentOuterPoint[3];
               localSegmentOuterPoint[0] = segment->x + segment->dXdZ * 10;
               localSegmentOuterPoint[1] = segment->y + segment->dYdZ * 10;

               localSegmentOuterPoint[2] = 10;
               localSegmentInnerPoint[0] = segment->x - segment->dXdZ * 10;
               localSegmentInnerPoint[1] = segment->y - segment->dYdZ * 10;

               localSegmentInnerPoint[2] = -10;
               matrix->LocalToMaster( localSegmentInnerPoint, globalSegmentInnerPoint );
               matrix->LocalToMaster( localSegmentOuterPoint, globalSegmentOuterPoint );

               segmentSet->AddLine(globalSegmentInnerPoint[0], globalSegmentInnerPoint[1], globalSegmentInnerPoint[2],
                                   globalSegmentOuterPoint[0], globalSegmentOuterPoint[1], globalSegmentOuterPoint[2] );
            }
         }
      }
      if ( !matches.empty() ) muonList->AddElement( segmentSet.release() );
      if (outerTrack) outerTrack->MakeTrack();
   }

   eviewer->GetGLViewer()->UpdateScene();
   eviewer->GetGLViewer()->CurrentCamera().Reset();

   ediFrame->AddFrame( new TGLabel(ediFrame, "Add missing info here."));
   ediFrame->MapSubwindows();
 }

REGISTER_FWDETAILVIEW(FWMuonDetailView);
