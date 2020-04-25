pub struct Animation<'mA, 'mMA, 'nA> {
    /* The name of the animation. If the modeling package this data was
     * exported from does support only a single animation channel, this
     * name is usually empty (length is zero).
     */
    m_name: Option<String>,
    // Duration of the animation in ticks
    m_duration: f64,
    // Ticks per second. Zero (0.000... ticks/second) if not
    // specified in the imported file
    m_ticks_per_second: Option<f64>,
    /* Number of bone animation channels.
       Each channel affects a single node.
       */
    m_num_channels: u64,
    /* Node animation channels. Each channel
       affects a single node. 
       ?? -> The array is m_num_channels in size.
       (maybe refine to a derivative type of usize?)
       */
    m_channels: &'nA NodeAnim,
    /* Number of mesh animation channels. Each
       channel affects a single mesh and defines
       vertex-based animation.
       */
    m_num_mesh_channels: u64,
    /* The mesh animation channels. Each channel
       affects a single mesh.
       The array is m_num_mesh_channels in size
       (maybe refine to a derivative of usize?)
       */
    m_mesh_channels: &'mA MeshAnim,
    /* The number of mesh animation channels. Each channel
       affects a single mesh and defines some morphing animation.
       */
    m_num_morph_mesh_channels: u64,
    /* The morph mesh animation channels. Each channel affects a single mesh.
       The array is mNumMorphMeshChannels in size.
       */
    m_morph_mesh_channels: &'mMA MeshMorphAnim    
}
pub struct NodeAnim {}
pub struct MeshAnim {}
pub struct MeshMorphAnim {}
