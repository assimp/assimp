package jassimp;

/**
 * @author Doug Stephen <a href="mailto:dstephen@ihmc.us">(dstephen@ihmc.us)</a>
 */
public class AiMetadataEntry
{
   public enum AiMetadataType
   {
      AI_BOOL, AI_INT32, AI_UINT64, AI_FLOAT, AI_DOUBLE, AI_AISTRING, AI_AIVECTOR3D
   }

   private AiMetadataType mType;
   private Object mData;

   public AiMetadataType getMetaDataType()
   {
      return mType;
   }

//   public void setMetaDataType(AiMetadataType type)
//   {
//      this.mType = type;
//   }

   public Object getData()
   {
      return mData;
   }

//   public void setData(Object data)
//   {
//      this.mData = data;
//   }
}
