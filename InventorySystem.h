// InventorySystem.cpp
#include "InventorySystem.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "TimerManager.h"

// ==================== РЕАЛИЗАЦИЯ КОМПОНЕНТА ИНВЕНТАРЯ ====================
UInventoryComponent::UInventoryComponent()
{
    PrimaryComponentTick.bCanEverTick = false;
    Slots.SetNum(MAX_INVENTORY_SLOTS);
}

bool UInventoryComponent::AddItem(UItemDefinition* Item, int32 QuantityToAdd)
{
    if (!Item || QuantityToAdd <= 0)
    {
        return false;
    }

    int32 RemainingQuantity = QuantityToAdd;

    // Сначала пытаемся добавить в существующие стаки
    for (int32 i = 0; i < Slots.Num(); ++i)
    {
        if (!Slots[i].IsEmpty() && Slots[i].Item == Item && Slots[i].Quantity < Item->MaxStackSize)
        {
            int32 SpaceInStack = Item->MaxStackSize - Slots[i].Quantity;
            int32 AddAmount = FMath::Min(RemainingQuantity, SpaceInStack);
            
            Slots[i].Quantity += AddAmount;
            RemainingQuantity -= AddAmount;
            
            BroadcastSlotChanged(i);
            
            if (RemainingQuantity <= 0)
            {
                return true;
            }
        }
    }

    // Затем добавляем в пустые слоты
    while (RemainingQuantity > 0)
    {
        int32 EmptySlot = GetFirstEmptySlot();
        
        if (EmptySlot == INDEX_NONE)
        {
            OnInventoryFull.Broadcast();
            return false;
        }
        
        int32 AddAmount = FMath::Min(RemainingQuantity, Item->MaxStackSize);
        
        Slots[EmptySlot].Item = Item;
        Slots[EmptySlot].Quantity = AddAmount;
        RemainingQuantity -= AddAmount;
        
        BroadcastSlotChanged(EmptySlot);
    }
    
    return true;
}

bool UInventoryComponent::RemoveItem(int32 SlotIndex, int32 QuantityToRemove)
{
    if (!Slots.IsValidIndex(SlotIndex) || Slots[SlotIndex].IsEmpty())
    {
        return false;
    }
    
    FInventorySlot& Slot = Slots[SlotIndex];
    
    if (Slot.Quantity < QuantityToRemove)
    {
        return false;
    }
    
    Slot.Quantity -= QuantityToRemove;
    
    if (Slot.Quantity <= 0)
    {
        Slot.Clear();
    }
    
    BroadcastSlotChanged(SlotIndex);
    return true;
}

bool UInventoryComponent::RemoveItemById(FName ItemId, int32 QuantityToRemove)
{
    int32 SlotIndex = FindItemSlot(ItemId);
    if (SlotIndex == INDEX_NONE)
    {
        return false;
    }
    
    return RemoveItem(SlotIndex, QuantityToRemove);
}

bool UInventoryComponent::MoveItem(int32 FromSlot, int32 ToSlot)
{
    if (!Slots.IsValidIndex(FromSlot) || !Slots.IsValidIndex(ToSlot) || FromSlot == ToSlot)
    {
        return false;
    }
    
    FInventorySlot& Source = Slots[FromSlot];
    FInventorySlot& Target = Slots[ToSlot];
    
    if (Source.IsEmpty())
    {
        return false;
    }
    
    // Если целевой слот пуст - просто перемещаем
    if (Target.IsEmpty())
    {
        Target = Source;
        Source.Clear();
        
        BroadcastSlotChanged(FromSlot);
        BroadcastSlotChanged(ToSlot);
        return true;
    }
    
    // Если одинаковые предметы - пробуем объединить
    if (Source.Item == Target.Item)
    {
        int32 SpaceInTarget = Target.Item->MaxStackSize - Target.Quantity;
        
        if (SpaceInTarget > 0)
        {
            int32 MoveAmount = FMath::Min(Source.Quantity, SpaceInTarget);
            Target.Quantity += MoveAmount;
            Source.Quantity -= MoveAmount;
            
            if (Source.Quantity <= 0)
            {
                Source.Clear();
            }
            
            BroadcastSlotChanged(FromSlot);
            BroadcastSlotChanged(ToSlot);
            return true;
        }
    }
    
    return false;
}

bool UInventoryComponent::SwapItems(int32 SlotA, int32 SlotB)
{
    if (!Slots.IsValidIndex(SlotA) || !Slots.IsValidIndex(SlotB) || SlotA == SlotB)
    {
        return false;
    }
    
    FInventorySlot Temp = Slots[SlotA];
    Slots[SlotA] = Slots[SlotB];
    Slots[SlotB] = Temp;
    
    BroadcastSlotChanged(SlotA);
    BroadcastSlotChanged(SlotB);
    
    return true;
}

FInventorySlot UInventoryComponent::GetSlot(int32 SlotIndex) const
{
    if (Slots.IsValidIndex(SlotIndex))
    {
        return Slots[SlotIndex];
    }
    return FInventorySlot();
}

int32 UInventoryComponent::GetFreeSlotCount() const
{
    int32 FreeSlots = 0;
    for (const FInventorySlot& Slot : Slots)
    {
        if (Slot.IsEmpty())
        {
            FreeSlots++;
        }
    }
    return FreeSlots;
}

bool UInventoryComponent::IsInventoryFull() const
{
    return GetFreeSlotCount() == 0;
}

int32 UInventoryComponent::GetItemQuantity(FName ItemId) const
{
    int32 Total = 0;
    for (const FInventorySlot& Slot : Slots)
    {
        if (Slot.Item && Slot.Item->GetFName() == ItemId)
        {
            Total += Slot.Quantity;
        }
    }
    return Total;
}

bool UInventoryComponent::HasItem(FName ItemId, int32 RequiredQuantity) const
{
    return GetItemQuantity(ItemId) >= RequiredQuantity;
}

void UInventoryComponent::ClearInventory()
{
    for (int32 i = 0; i < Slots.Num(); ++i)
    {
        if (!Slots[i].IsEmpty())
        {
            Slots[i].Clear();
            BroadcastSlotChanged(i);
        }
    }
}

int32 UInventoryComponent::GetFirstEmptySlot() const
{
    for (int32 i = 0; i < Slots.Num(); ++i)
    {
        if (Slots[i].IsEmpty())
        {
            return i;
        }
    }
    return INDEX_NONE;
}

int32 UInventoryComponent::FindItemSlot(FName ItemId) const
{
    for (int32 i = 0; i < Slots.Num(); ++i)
    {
        if (Slots[i].Item && Slots[i].Item->GetFName() == ItemId && !Slots[i].IsEmpty())
        {
            return i;
        }
    }
    return INDEX_NONE;
}

void UInventoryComponent::BroadcastSlotChanged(int32 SlotIndex)
{
    if (Slots.IsValidIndex(SlotIndex))
    {
        FInventorySlot& Slot = Slots[SlotIndex];
        OnInventoryChanged.Broadcast(SlotIndex, Slot.Item, Slot.Quantity);
    }
}

// ==================== РЕАЛИЗАЦИЯ АКТОРА ДЛЯ ПОДБОРА ====================
APickupBase::APickupBase()
{
    PrimaryActorTick.bCanEverTick = true;
    
    MeshComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("MeshComponent"));
    RootComponent = MeshComponent;
    MeshComponent->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
    MeshComponent->SetCollisionResponseToAllChannels(ECR_Overlap);
    MeshComponent->SetGenerateOverlapEvents(true);
    
    CollisionComponent = CreateDefaultSubobject<USphereComponent>(TEXT("CollisionComponent"));
    CollisionComponent->SetupAttachment(RootComponent);
    CollisionComponent->SetSphereRadius(100.0f);
    CollisionComponent->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
    CollisionComponent->SetCollisionResponseToAllChannels(ECR_Overlap);
    
    StartLocation = FVector::ZeroVector;
    RunningTime = 0.0f;
}

void APickupBase::BeginPlay()
{
    Super::BeginPlay();
    StartLocation = GetActorLocation();
    
    if (MeshComponent && ItemDefinition && !ItemDefinition->WorldMesh.IsNull())
    {
        UStaticMesh* LoadedMesh = ItemDefinition->WorldMesh.LoadSynchronous();
        if (LoadedMesh)
        {
            MeshComponent->SetStaticMesh(LoadedMesh);
        }
    }
}

void APickupBase::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);
    
    RunningTime += DeltaTime;
    
    // Эффект парения
    float DeltaZ = FMath::Sin(RunningTime * FloatSpeed) * FloatAmplitude;
    FVector NewLocation = StartLocation;
    NewLocation.Z += DeltaZ;
    SetActorLocation(NewLocation);
    
    // Эффект вращения
    FRotator NewRotation = GetActorRotation();
    NewRotation.Yaw += RotationSpeed * DeltaTime;
    SetActorRotation(NewRotation);
}

void APickupBase::NotifyActorBeginOverlap(AActor* OtherActor)
{
    Super::NotifyActorBeginOverlap(OtherActor);
    
    if (!OtherActor || !ItemDefinition)
    {
        return;
    }
    
    UInventoryComponent* Inventory = OtherActor->FindComponentByClass<UInventoryComponent>();
    
    if (Inventory)
    {
        if (Inventory->AddItem(ItemDefinition, Quantity))
        {
            PlayPickupEffect();
            
            if (bAutoDestroyOnPickup)
            {
                Destroy();
            }
        }
        else
        {
            // Вывод сообщения в консоль и на экран
            FString Message = FString::Printf(TEXT("Cannot pickup %s - inventory is full (max 4 items)!"), 
                *ItemDefinition->ItemName.ToString());
            
            UE_LOG(LogTemp, Warning, TEXT("%s"), *Message);
            
            if (GEngine)
            {
                GEngine->AddOnScreenDebugMessage(-1, 2.0f, FColor::Red, Message);
            }
        }
    }
}

void APickupBase::InitializeFromItem(UItemDefinition* Item, int32 InQuantity)
{
    ItemDefinition = Item;
    Quantity = InQuantity;
    
    if (MeshComponent && ItemDefinition && !ItemDefinition->WorldMesh.IsNull())
    {
        UStaticMesh* LoadedMesh = ItemDefinition->WorldMesh.LoadSynchronous();
        if (LoadedMesh)
        {
            MeshComponent->SetStaticMesh(LoadedMesh);
        }
    }
}

void APickupBase::PlayPickupEffect()
{
    // Эффект уменьшения перед уничтожением
    FVector OriginalScale = GetActorScale3D();
    SetActorScale3D(OriginalScale * 0.5f);
    
    FTimerHandle TimerHandle;
    GetWorld()->GetTimerManager().SetTimer(TimerHandle, [this, OriginalScale]()
    {
        SetActorScale3D(OriginalScale);
    }, 0.1f, false);
    
    OnPickupCollected.Broadcast(this);
}